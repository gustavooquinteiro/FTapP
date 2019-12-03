#include "../include/transport.h"
#include "../include/network.h"
#include "../include/requisition_queue.h"
#include "../include/pkg_queue.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#define PKG_SIZE    25
#define DATA_SIZE   14
#define MAX_PORT    65535
#define MAX_SOCKETS 100
#define LISTENER_ID 100
#define WINDOW_SIZE 20
#define TRUE 1
#define FALSE 0

#define ALLOC(X) (X*) malloc(sizeof(X))
              
#define MAX_SOCKETS_ERROR       "Sockets limit reached."
#define CONNECTION_EXISTS_ERROR "Connection already exists"
#define LISTENER_ERROR          "Listener already exists."
#define ID_LISTENER_ERROR       "Not a listener"
#define MUTEX_ERROR             "Thread mutex init fail."
#define NULL_ERROR              "Pointer is null."
#define THREAD_CANCEL_ERROR     "Cancel thread error."

// Struct do connection socket da camada de transporte
typedef struct connection_socket{
    uint16_t    local_port;
    uint16_t    remote_port;
    char        remote_IP[16];    
    PackageQueue*  pkg_queue;
    PackageQueue*  ack_queue;
    uint8_t sending;
    pthread_mutex_t queue_lock;
} ConnectionSocket;

// Struct do listener socket da camada de transporte
typedef struct listener_socket{
    uint16_t     port;
    PackageQueue*   req_pkg_queue; 
    Queue*          req_ip_queue;

    PackageQueue*   pkg_queue; 
    Queue*          pkg_ip_queue;  
    pthread_mutex_t req_queue_lock;
    pthread_mutex_t queue_lock;    
    pthread_t       thread;   
} ListenerSocket;

// Estrutura que armazena os sockets
ConnectionSocket*   sockets[MAX_SOCKETS];
ListenerSocket*     listener_socket = NULL;
pthread_t           trans_thread;

// Struct do segmento da camada de transporte
typedef struct package{
    uint32_t    seq_number;
    uint16_t    orig_port;
    uint16_t    dest_port;
    uint16_t    checksum;
    uint8_t     flags;
    uint8_t     data[DATA_SIZE];
} Package;

// Thread que a camada de transporte vai estar sempre executando
void* listen_network_layer(void* data){
    int nada;   
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &nada);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &nada);

    while(TRUE){
        // Recebe um segmento da camada de rede
        int result;
        char remoteIP[16];
        Package* pkg = (Package*) malloc(sizeof(Package));
        result = recv_segment((char*)pkg, sizeof(pkg), remoteIP);

        // Acha qual conexão está relacionada com este segmento
        int found = 0;
        for (int i = 0; i < MAX_SOCKETS; ++i)
        {
            if( sockets[i] != NULL &&
                sockets[i]->local_port == pkg->dest_port &&
                sockets[i]->remote_port == pkg->orig_port &&
                strcmp(sockets[i]->remote_IP, remoteIP) == 0)
            {
                pthread_mutex_lock(&sockets[i]->queue_lock);
                if(pkg->ack_flag()){
                    pkg_queue_push(sockets[i]->ack_queue, pkg);
                }else{
                    pkg_queue_push(sockets[i]->pkg_queue, pkg);
                }                
                pthread_mutex_unlock(&sockets[i]->queue_lock);
                found = 1;
                break;
            }
        }


        // Se nao achou conexao, e existe listener,
        // joga pacote pro listener
        if(!found && listener_socket != NULL){
            pthread_mutex_lock(&listener_socket->queue_lock);
            pkg_queue_push(listener_socket->pkg_queue, pkg);
            qpush(listener_socket->pkg_ip_queue, remoteIP);
            pthread_mutex_unlock(&listener_socket->queue_lock);
        }        
    }
}
// Inicializa execução da camada de transporte
int transport_init(){
    // Inicializa vetor de sockets
    for (int i = 0; i < MAX_SOCKETS; ++i)
    {
        sockets[i] = NULL;
    }

    // Ouve da camada de rede
    pthread_create(&trans_thread, NULL, listen_network_layer, NULL);

    return 1;
}

// Verifica se um segmento está corrompido
int is_corrupted(Package* pkg){
    uint32_t aux;
    uint16_t sum = 0;

    uint8_t* data;
    data = (uint8_t*) pkg;

    for (int i = 1; i <= 13; i+=2)
    {
        uint16_t operand = pkg->data[i];
        operand = operand << 8;
        operand |= pkg->data[i+1];
        aux = (uint32_t)sum + (uint32_t)operand;
        if(aux >> 16 == 1){
            aux += 1;
        }
        sum = aux;
    }

    if(sum == 0xffff) return FALSE;
    else return TRUE;
}

int fin_flag(Package* pkg){
    return (pkg->flags & 0b00000001);
}
int syn_flag(Package* pkg){
    return (pkg->flags & 0b00000010);
}
int ack_flag(Package* pkg){
    return (pkg->flags & 0b00000100);
}

// Thread que o socket listener, caso exista, vai estar sempre executando
void* listen_requisitions(void* data){
    ListenerSocket* listener = (ListenerSocket*) data; 
    int nada;   
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &nada);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &nada);

    while(TRUE){
        char clientIP[16];
        Package* pkg;
        
        // Aguarda a fila ter pelo menos um pacote
        while(pkg_queue_isempty(listener->pkg_queue));

        // Retira pacote da fila e pega o IP relacionado a ele
        // na fila de ips
        pthread_mutex_lock(&listener->queue_lock);
        pkg = pkg_queue_pop(listener->pkg_queue);
        strcpy(clientIP, qfront(listener->pkg_ip_queue));
        qpop(listener->pkg_ip_queue);
        pthread_mutex_unlock(&listener->queue_lock);

        // Se não tiver corrompido e for uma tentativa de conexao,
        // coloca na fila de requisições
        if(!is_corrupted(pkg) && syn_flag(pkg)){
            pthread_mutex_lock(&listener->req_queue_lock);
            qpush(listener->req_ip_queue, clientIP);
            pkg_queue_push(listener->req_pkg_queue, pkg);
            pthread_mutex_unlock(&listener->req_queue_lock);
        }else{
            // Joga o pacote fora
            free(pkg); 
        }              
    }
}

// Cria um listener socket
int new_listener_socket(int port){    
    if(listener_socket != NULL){
        perror(LISTENER_ERROR);
        return -1;
    }

    // Inicializando os valores do listener
    listener_socket = (ListenerSocket*) malloc(sizeof(ListenerSocket));    
    listener_socket->port = port;
    listener_socket->req_ip_queue = defineQueue();
    listener_socket->pkg_queue = new_pkg_queue();

    // Cria mutex da thread para evitar erro
    // na manipulação da fila de requisições
    if (pthread_mutex_init(&listener_socket->req_queue_lock, NULL) != 0)
    {
        printf(MUTEX_ERROR);
        free(listener_socket);
        return -1;
    }
    // Cria mutex da thread para evitar erro
    // na manipulação da fila de pacotes
    if (pthread_mutex_init(&listener_socket->queue_lock, NULL) != 0)
    {
        printf(MUTEX_ERROR);
        free(listener_socket);
        return -1;
    }

    // Cria thread para escutar requisições
    pthread_create(&listener_socket->thread, NULL, listen_requisitions, (void *)listener_socket);

    // Retorna o índice do listener 
    // (tem que ser maior que o índice máximo)
    return LISTENER_ID;
}

int new_connection_socket(int listener_id){
    // Erro se o id passado não for o id especial de listener,
    if(listener_id != LISTENER_ID){
        perror(ID_LISTENER_ERROR);
        return -1;
    }
    // Erro se o listener não estiver criado
    if(listener_socket == NULL){
        perror(NULL_ERROR);
        return -1;
    }
    char ip[16];

    // Loop esperando a fila de requisições ter pelo menos uma requisição
    while(qisEmpty(listener_socket->req_ip_queue));    
    
    // Guarda e retira o próximo da fila
    pthread_mutex_lock(&listener_socket->req_queue_lock);    
    strcpy(ip, qfront(listener_socket->req_ip_queue));
    qpop(listener_socket->req_ip_queue);
    Package* requisition = pkg_queue_pop(listener_socket->req_pkg_queue);
    pthread_mutex_unlock(&listener_socket->req_queue_lock);   

    // Cria o socket de conexão, baseado no ip 
    // da próxima requisição de listener
    for (int i = 0; i < MAX_SOCKETS; ++i)
    {
        if( sockets[i] != NULL &&
            sockets[i]->local_port == requisition->dest_port &&
            sockets[i]->remote_port == requisition->orig_port &&
            strcmp(sockets[i]->remote_IP, ip) == 0)
        {
            perror(CONNECTION_EXISTS_ERROR);
            free(requisition);
            return -1;
        }
    }

    for (int i = 0; i < MAX_SOCKETS; ++i)
    {
        if(sockets[i] == NULL)
        {
            sockets[i] = ALLOC(ConnectionSocket);
            sockets[i]->local_port = requisition->dest_port;
            sockets[i]->remote_port = requisition->orig_port;
            strcpy(sockets[i]->remote_IP, ip);
            sockets[i]->pkg_queue = new_pkg_queue();
            if (pthread_mutex_init(&sockets[i]->queue_lock, NULL) != 0)
            {
                free(sockets[i]);
                printf(MUTEX_ERROR);               
                return -1;
            }
            free(requisition);
            return i;
        }
    }

    perror(MAX_SOCKETS_ERROR);
    return -1;   
}

int new_requester_socket(uint16_t port, const char* ip){   

    // Procura uma porta que nao entre em conflito
    // com as conexões que já existem
    uint16_t local_port;
    int index = -1;
    while(TRUE){
        int found = 1;
        srand((unsigned)time(NULL));
        local_port = rand() % MAX_PORT;
        for (int i = 0; i < MAX_SOCKETS; ++i)
        {   
            if(sockets[i] == NULL){
                index = i;                
            }
            if( sockets[i] != NULL &&
                sockets[i]->local_port == local_port &&
                sockets[i]->remote_port == port &&
                strcmp(sockets[i]->remote_IP, ip) == 0)
            {
                found = 0;
                break;
            }
        }
        if(found) break;
    }

    if(index == -1){
        perror(MAX_SOCKETS_ERROR);
        return -1;
    }

    // Cria o socket de conexão, baseado nos argumentos passados,
    // com seu id adequado
    sockets[index] = ALLOC(ConnectionSocket);
    sockets[index]->local_port = local_port;
    sockets[index]->remote_port = port;
    strcpy(sockets[index]->remote_IP, ip);
    sockets[index]->pkg_queue = new_pkg_queue();
    if (pthread_mutex_init(&sockets[index]->queue_lock, NULL) != 0)
    {
        free(sockets[index]);
        printf(MUTEX_ERROR);                
        return -1;
    }
    return index;    
}

int get_peer_ip(ConnectionSocket* socket, char* ip){
    strcpy(ip, socket->remote_IP);
    return 1;
}

// Depende da criação (new)
void delete_listener_socket(ListenerSocket* listener){
    if(listener == NULL){
        perror(NULL_ERROR);
        return;
    }

    if(pthread_cancel(listener->thread) != 0){
        perror(THREAD_CANCEL_ERROR);
    } 
      
    pthread_join(listener->thread, NULL);
    pthread_mutex_destroy(&listener->queue_lock);
    clearQueue(listener->req_ip_queue);
    clearQueue(listener->pkg_ip_queue);
    clear_queue(listener->pkg_queue);
    clear_queue(listener->req_pkg_queue);
    free(listener);
}

void delete_connection_socket(ConnectionSocket* socket){
    if(socket == NULL){
        perror(NULL_ERROR);
        return;
    }
    clear_queue(socket->pkg_queue);
    pthread_mutex_destroy(&socket->queue_lock);
    free(socket);
}

// FUNÇÕES PENDENTES
// Dependente do protocolo


// Funções do remetente

volatile int timeout = FALSE;
int stop = FALSE;
int next_seq_number = 0;
int base = 0;
Package* window[WINDOW_SIZE];


void set_timeout(int sig){
    timeout = TRUE;
}

void stop_timer(){
    stop = TRUE;    
}

void start_timer(){
    timeout = FALSE;
    signal(SIGALRM, set_timeout);
    alarm(5);
}

int getacknum(Package* pkg){
    return pkg->seq_number;
}


void * snd_receiver(void * args){
    ConnectionSocket* socket = (ConnectionSocket* )args;
    while(TRUE){
        // Espera ter pelo menos um pacote na fila de acks
        while(pkg_queue_isempty(socket->ack_queue));        
        // Pega esse pacote
        pthread_mutex_lock(&socket->queue_lock);
        Package * ack = pkg_queue_pop(socket->ack_queue);
        pthread_mutex_unlock(&socket->queue_lock);
        
        // Faz as coisas do protocolo
        for(int i = 0; i < (getacknum(ack)+1)-base; i++){
            free(window[i]);
        }        
        base = getacknum(ack) + 1;
        if (!is_corrupted(ack)){
            if (base == next_seq_number){
                stop_timer();
            }else{
                start_timer();
            }
        }
        free(ack); 
    }
}


int send_message(ConnectionSocket* socket, char* snd_data, int length){
    if (socket->sending){
        return -1;
    }
    socket->sending = TRUE;
    pthread_t snd_receiver_thread;
    pthread_create(&snd_receiver_thread, NULL, snd_receiver, (void *)socket);
    while(iterator < length){
        
        if (next_seq_number < base + WINDOW_SIZE){            
            // Criar pacote
            Package* pkg = ALLOC(Package);    
            pkg->seq_number = next_seq_number;
            pkg->orig_port = socket->local_port;
            pkg->dest_port = socket->remote_port;
            pkg->flags = 0x00;
            strncpy(pkg->data, snd_data + iterator, DATA_SIZE);
            pkg->checksum = make_checksum(pkg);
            
            // Envia pacote
            window[next_seq_number-base] = pkg;
            send_segment((char*) pkg, sizeof(pkg), socket->remoteIP);
            if (base == next_seq_number){
                start_timer();
            }            
            next_seq_number += 1;            
        }
        
        if(timeout){
            start_timer();
            // Manda todo mundo de novo
            for(int i = 0; i < (next_seq_number-base); i++){
                send_segment((char*) window[i], sizeof(window[i]), socket->remoteIP);
            }
        }

        iterator += DATA_SIZE;
    }
    if(DATA_SIZE - (iterator-length) != 0){
        // Envia o restinho
    }
    if(pthread_cancel(snd_receiver_thread != 0){
        perror(THREAD_CANCEL_ERROR);
    } 
    pthread_join(snd_receiver_thread, NULL);
    
    return 1;
}


// Funções do destinatário

int receive_message(ConnectionSocket* socket, char* rcv_data, int length){
    int expected_seq_number = 0;
    int iterator = 0;
    
    while (iterator < length){
        // Espera ter pelo menos um pacote na fila de acks
        while(pkg_queue_isempty(socket->pkg_queue));        
        // Pega esse pacote
        pthread_mutex_lock(socket->queue_lock);
        Package * pkg = pkg_queue_pop(socket->pkg_queue);
        pthread_mutex_unlock(socket->queue_lock);
        if (!is_corrupted(pkg) && pkg->seq_number == expected_seq_number){
            strncpy(rcv_data + iterator, pkg->data, DATA_SIZE);
            Package  * ack = ALLOC(Package);
            ack->seq_number = expected_seq_number;
            ack->flags = 0;
            set_ack_flag(ack);
            ack->orig_port = socket->local_port; 
            ack->dest_port = socket->remote_port;
            ack->checksum = make_checksum(ack);
            send_segment ((char *)ack, sizeof(ack), socket->remote_IP);
            expected_seq_number += 1;
        }
    }
    return 1;
    
}

// int send_message(tcp_socket* sock, void* snd_data, unsigned int length){
//     int size = send(sock->id, snd_data, length, SEND_FLAGS);
//     return size;
// }

// int recieve_message(tcp_socket* sock, void* rcv_data, unsigned int length, int flag){
//     int size;
//     if(flag == 0)
//         size = recv(sock->id, rcv_data, length, 0);
//     else 
//         size = recv(sock->id, rcv_data, length, RECV_FLAGS);
    
//     return size;
// }
