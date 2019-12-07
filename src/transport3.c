#include "../include/transport.h"
#include "../include/network.h"
#include "../include/requisition_queue.h"
#include "../include/pkg_queue.h"
#include "../include/data_queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#define PKG_SIZE    26
#define DATA_SIZE   15
#define MAX_PORT    65535
#define MAX_SOCKETS 100
#define LISTENER_ID 100
#define WINDOW_SIZE 20
#define TIME 2000
#define TRUE 1
#define FALSE 0

#define ALLOC(X) (X*) malloc(sizeof(X))
              
#define MAX_SOCKETS_ERROR       "Sockets limit reached."
#define CONNECTION_EXISTS_ERROR "Connection already exists"
#define LISTENER_ERROR          "Listener already exists."
#define ID_LISTENER_ERROR       "Not a listener"
#define MUTEX_ERROR             "Thread mutex init fail."
#define MUTEX_LOCK_ERROR        "Thread mutex lock fail."
#define MUTEX_UNLOCK_ERROR      "Thread mutex unlock fail."
#define NULL_ERROR              "Pointer is null.\n"
#define THREAD_CANCEL_ERROR     "Cancel thread error."


#define ACK_FLAG 0b00000100
#define SYN_FLAG 0b00000010
#define FIN_FLAG 0b00000001
#define END_FLAG 0b00001000

// Struct do connection socket da camada de transporte
typedef struct connection_socket{
    int timeout;
    int stop_timer;
    int start_timer;

    int receiving;
    int sending;
    uint32_t end_of_message;

    uint16_t    local_port;
    uint16_t    remote_port;
    char        remote_IP[16];    
    PackageQueue*  pkg_queue;
    DataQueue*  data_queue;
    pthread_mutex_t queue_lock;
    pthread_mutex_t data_queue_lock;
    
    pthread_t snd_thread;
    pthread_t rcv_thread;
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

// Variável que indica se quem está manipulando é o cliente
int REAL_SENDER_PORT;
int REAL_RECEIVER_PORT;
int USER_TYPE;

// Struct do segmento da camada de transporte
typedef struct package{
    uint32_t    seq_number;
    uint16_t    orig_port;
    uint16_t    dest_port;    
    uint8_t     flags;
    uint8_t     data[DATA_SIZE];
    uint16_t    checksum;
} Package;


void print_pack(Package* pkg, char* funcName){
    printf("============\n");
    printf("%s: |SeqNumber = %u |\n", funcName, pkg->seq_number);
    printf("%s: |OrigPort = %u |\n", funcName, pkg->orig_port);
    printf("%s: |DestPort = %u |\n", funcName, pkg->dest_port);
    printf("%s: |Flags = 0x%x |\n", funcName, pkg->flags);
    printf("%s: |Data = ", funcName);
    for (int i = 0; i < DATA_SIZE; ++i)
    {
        if(33 <= pkg->data[i] &&  pkg->data[i] <= 126 && pkg->data[i] != ' ')
            printf("%c ", pkg->data[i]);
        else if(pkg->data[i] == ' ')
            printf("[SPC] ");
        else
            printf("0x%x ", pkg->data[i]);
    }
    printf("|\n");
    printf("============\n");
}

void* sender_thread(void* args);
void* receiver_thread(void* args);

int get_peer_ip(int socket_id, char* ip){
    ConnectionSocket* socket = sockets[socket_id];
    strcpy(ip, socket->remote_IP);
    return 1;
}

uint32_t base = 1;
uint32_t nextseqnum = 1;

ConnectionSocket* make_socket(int index, uint16_t local_port, uint16_t remote_port, const char* remote_IP){
    ConnectionSocket* socket = ALLOC(ConnectionSocket);
    socket->stop_timer = FALSE;
    socket->start_timer = FALSE;
    socket->timeout = FALSE;
    
    socket->local_port = local_port;
    socket->remote_port = remote_port;       
    strcpy(socket->remote_IP, remote_IP);

    socket->receiving = FALSE;
    socket->sending = FALSE;
    socket->end_of_message = -1;

    socket->pkg_queue = new_pkg_queue();
    socket->data_queue = new_data_queue();

    if (pthread_mutex_init(&socket->queue_lock, NULL) != 0)
    {
        free(socket);
        printf(MUTEX_ERROR);               
        return NULL;
    }
    if (pthread_mutex_init(&socket->data_queue_lock, NULL) != 0)
    {
        free(socket);
        printf(MUTEX_ERROR);               
        return NULL;
    }
    
    return socket;
}

void make_checksum(Package* pkg){
    pkg->checksum = 0;
    
    uint32_t aux;
    uint16_t sum = 0;    
    uint8_t* data;
    data = (uint8_t*) pkg;
    
    for(int i = 0; i < PKG_SIZE; i+=2){
        // printf("data[%i] = %x\n", i, data[i]);
        // printf("data[%i] = %x\n", i+1, data[i+1]);
        uint16_t operand = data[i+1];
        operand = operand << 8;
        operand |= data[i];
        // printf("operando = %x\n", operand);
        aux = (uint32_t)sum + (uint32_t)operand;
        if(aux >> 16 ==1){
            aux += 1;
        }
        sum = aux;
    }
    
    pkg->checksum = ~sum;
}

Package* make_pkg(uint32_t seq_number, uint16_t orig_port, uint16_t dest_port, uint16_t flags, uint8_t* data){
    Package* pkg = ALLOC(Package);
    pkg->seq_number = seq_number;
    pkg->orig_port = orig_port;
    pkg->dest_port = dest_port;
    pkg->flags = flags;
    if(data != NULL){
        memcpy((void*)pkg->data, (void*)data, DATA_SIZE);
    }
    make_checksum(pkg);

    return pkg;
}


// Verifica se um segmento está corrompido
int is_corrupted(Package* pkg){
    uint32_t aux;
    uint16_t sum = 0;

    uint8_t* data;
    data = (uint8_t*) pkg;

    for (int i = 0; i < PKG_SIZE ; i+=2)
    {
        // printf("data[%i] = %x\n", i, data[i]);
        // printf("data[%i] = %x\n", i+1, data[i+1]);
        uint16_t operand = data[i+1];
        operand = operand << 8;
        operand |= data[i];
        // printf("operando = %x\n", operand);
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
    return (pkg->flags & FIN_FLAG);
}
int syn_flag(Package* pkg){
    return (pkg->flags & SYN_FLAG);
}
int ack_flag(Package* pkg){
    return (pkg->flags & ACK_FLAG);
}
int end_flag(Package* pkg){
    return (pkg->flags & END_FLAG);
}

void set_ack_flag(Package* pkg){
    pkg->flags |= 0b00000100;
}
void set_syn_flag(Package* pkg){
    pkg->flags |= 0b00000010;
}

// Thread que a camada de transporte vai estar sempre executando
void* listen_network_layer(void* data){
    int nada;   
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &nada);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &nada);

    printf("TRANSPORT: Começou a ouvir.\n");
    while(TRUE){        
        // Recebe um segmento da camada de rede
        int result;
        char remoteIP[16];
        Package* pkg = (Package*) malloc(sizeof(Package));
        
        result = recv_segment((char*)pkg, sizeof(Package), remoteIP, REAL_RECEIVER_PORT);
        printf("TRANSPORT: Chegou algo.\n");
        
        if(result == FALSE){
            perror("TRANSPORT: Receive error");
            free(pkg);
            continue;
        }

        // printf("TRANSPORT: Package DestPort = %u\n", pkg->dest_port);
        // printf("TRANSPORT: Package OrigPort = %u\n", pkg->orig_port);
        // printf("TRANSPORT: Package IP = %s\n", remoteIP);
        // print_pack(pkg, "TRANSPORT");
        
        // Acha qual conexão está relacionada com este segmento
        int found = 0;
        for (int i = 0; i < MAX_SOCKETS; ++i)
        {
            if( sockets[i] != NULL &&
                sockets[i]->local_port == pkg->dest_port &&
                sockets[i]->remote_port == pkg->orig_port &&
                strcmp(sockets[i]->remote_IP, remoteIP) == 0)
            {
                printf("TRANSPORT: Achou conexão relacionda, socket %i.\n", i);
                pthread_mutex_lock(&sockets[i]->queue_lock);
                pkg_queue_push(sockets[i]->pkg_queue, pkg);             
                pthread_mutex_unlock(&sockets[i]->queue_lock);
                found = 1;
                break;
            }
        }

        if(!found){
            printf("TRANSPORT: Não achou conexão relacionda.\n");
        }

        // Se nao achou conexao, e existe listener,
        // joga pacote pro listener
        if(!found && listener_socket != NULL){
            if(pthread_mutex_lock(&listener_socket->queue_lock) != 0) perror(MUTEX_LOCK_ERROR);
            // printf("TRANSPORT: Lockou o mutex.\n");
            pkg_queue_push(listener_socket->pkg_queue, pkg);
            // printf("TRANSPORT: Deu push na fila de pacotes.\n");
            qpush(listener_socket->pkg_ip_queue, remoteIP);
            // printf("TRANSPORT: Deu push na fila de IPs.\n");
            if(pthread_mutex_unlock(&listener_socket->queue_lock) != 0) perror(MUTEX_UNLOCK_ERROR);
            // printf("TRANSPORT: Unlockou o mutex.\n");
        }

        // printf("TRANSPORT: Continua ouvindo.\n");       
    }
}
// Inicializa execução da camada de transporte
int transport_init(int user_type){
    // Inicializa CLIENT
    USER_TYPE = user_type;
    if(user_type == CLIENT){
        REAL_SENDER_PORT = 8074;
        REAL_RECEIVER_PORT = 8075;
    }
    else if(user_type == SERVER){
        REAL_SENDER_PORT = 8075;
        REAL_RECEIVER_PORT = 8074;
    }
    else{
        return -1;
    }

    // Inicializa vetor de sockets
    for (int i = 0; i < MAX_SOCKETS; ++i)
    {
        sockets[i] = NULL;
    }

    // Ouve da camada de rede
    pthread_create(&trans_thread, NULL, listen_network_layer, NULL);

    return 1;
}

// Thread que o socket listener, caso exista, vai estar sempre executando
void* listen_requisitions(void* data){
    ListenerSocket* listener = (ListenerSocket*) data; 
    int nada;   
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &nada);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &nada);

    while(TRUE){
        // printf("LISTENER: Começou a ouvir.\n");
        char clientIP[16];
        Package* pkg;
        
        // Aguarda a fila ter pelo menos um pacote
        while(pkg_queue_isempty(listener->pkg_queue));
        // printf("LISTENER: Chegou algo.\n");

        // Retira pacote da fila e pega o IP relacionado a ele
        // na fila de ips
        if(pthread_mutex_lock(&listener->queue_lock) != 0) perror(MUTEX_LOCK_ERROR);
        // printf("LISTENER: Lockou o mutex.\n");
        pkg = pkg_queue_pop(listener->pkg_queue);
        // printf("LISTENER: Copiou e tirou da fila de pacotes.\n");
        strcpy(clientIP, qfront(listener->pkg_ip_queue));
        // printf("LISTENER: Copiou da fila de IPs.\n");
        qpop(listener->pkg_ip_queue);
        // printf("LISTENER: Tirou da fila de IPs.\n");
        if(pthread_mutex_unlock(&listener->queue_lock) != 0) perror(MUTEX_UNLOCK_ERROR);

        // printf("LISTENER: Retirou da fila pacote com ip = %s.\n", clientIP);

        // printf("seq_number = 0x%x\n", pkg->seq_number);
        // printf("orig_port = 0x%x\n", pkg->orig_port);
        // printf("dest_port = 0x%x\n", pkg->dest_port);
        // printf("checksum = 0x%x\n", pkg->checksum);
        // printf("flags = 0x%x\n", pkg->flags);
        // for (int i = 0; i < DATA_SIZE; ++i)
        // {
        //     printf("0x%x ", pkg->data[i]);
        // }
        // printf("\n\n");

        // Se não tiver corrompido e for uma tentativa de conexao,
        // coloca na fila de requisições
        if(!is_corrupted(pkg)){
            // printf("LISTENER: Não está corrompido.\n");
            if(syn_flag(pkg)){
                // printf("LISTENER: É tentativa de conexão.\n");
                pthread_mutex_lock(&listener->req_queue_lock);
                qpush(listener->req_ip_queue, clientIP);
                pkg_queue_push(listener->req_pkg_queue, pkg);
                pthread_mutex_unlock(&listener->req_queue_lock);
            }else{
                // printf("LISTENER: NÃO é tentativa de conexão.\n");
                free(pkg);
            }
        }else{
            // printf("LISTENER: Está corrompido.\n");
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
    listener_socket = ALLOC(ListenerSocket);
    listener_socket->port = port;
    listener_socket->req_pkg_queue = new_pkg_queue();
    listener_socket->req_ip_queue = defineQueue();
    listener_socket->pkg_queue = new_pkg_queue();
    listener_socket->pkg_ip_queue = defineQueue();
    

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

volatile int timeout_requisition = FALSE;
void set_timeout_requisition(int sig){
    timeout_requisition = TRUE;
}



int new_connection_socket(int listener_id){
    // Erro se o id passado não for o id especial de listener,
    if(listener_id != LISTENER_ID){
        perror(ID_LISTENER_ERROR);
        return -1;
    }
    // Erro se o listener não estiver criado
    if(listener_socket == NULL){
        printf("NEW_CONNECTION: ");
        printf(NULL_ERROR);
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
    int index = -1;
    for (int i = 0; i < MAX_SOCKETS; ++i)
    {
        if(sockets[i] == NULL){
            index = i;                
        }
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

    if(index == -1){
        perror(MAX_SOCKETS_ERROR);
        return -1; 
    }

    // Cria o socket de conexão
    sockets[index] = make_socket(index, requisition->dest_port, requisition->orig_port, ip);    
    free(requisition);
    if(sockets[index] == NULL){
        return -1;
    }

    printf("NEW_CONNECTION: Criou o socket\n");
    printf("NEW_CONNECTION: Socket LocalPort = %u\n", sockets[index]->local_port);
    printf("NEW_CONNECTION: Socket RemotePort = %u\n", sockets[index]->remote_port);
    printf("NEW_CONNECTION: Socket IP = %s\n", sockets[index]->remote_IP);
    
    // Envia SYNACK
    // Cria o pacote SYNACK
    uint16_t local_port = sockets[index]->local_port;
    uint16_t remote_port = sockets[index]->remote_port;
    Package* synack = make_pkg(0, local_port, remote_port, ACK_FLAG, NULL);

    int tries = 0;
    while(tries < 15){
        // Envia o SYNACK
        send_segment((char*)synack, sizeof(Package), ip, REAL_SENDER_PORT);
        printf("NEW_CONNECTION: Tentativa %i: Enviou o SYNACK para %s.\n", tries+1, ip);
        // Aguarda chegar a respostasend com timeout de 2 segundos
        timeout_requisition = FALSE;
        signal(SIGALRM, set_timeout_requisition);
        alarm(2);
        while(pkg_queue_isempty(sockets[index]->pkg_queue) && !timeout_requisition);
       
        if(!timeout_requisition){
            // Pega esse pacote
            
            Package * pkg = pkg_queue_front(sockets[index]->pkg_queue);            
            if (!is_corrupted(pkg)){
                if(!syn_flag(pkg)){
                    printf("NEW_CONNECTION: Recebeu a tréplica.\n");
                    // Começa a thread do socket
                    break;
                }
            }else{
                pthread_mutex_lock(&sockets[index]->queue_lock);
                pkg_queue_pop(sockets[index]->pkg_queue);
                pthread_mutex_unlock(&sockets[index]->queue_lock);                
            }
            free(pkg);
        }
        printf("NEW_CONNECTION: Timeout\n");
        tries++;        
    }

    if(tries >= 15){
        delete_connection_socket(index);
        free(synack);
        return -1;
    }

    // E começa a rodar o socket
    sockets[index]->receiving = TRUE;
    printf("NEW_CONNECTION: Receiving = TRUE.\n");
    pthread_create(&sockets[index]->snd_thread, NULL, sender_thread, (void*)sockets[index]);
    pthread_create(&sockets[index]->rcv_thread, NULL, receiver_thread, (void*)sockets[index]);

    return index;
}

int new_requester_socket(uint16_t port, const char* ip){
    printf("NEW_REQUESTER: Pediu para criar socket.\n");
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
    sockets[index] = make_socket(index, local_port, port, ip);
    if(sockets[index] == NULL) return -1;

    printf("NEW_REQUESTER: Criou o socket\n");
    printf("NEW_REQUESTER: Socket LocalPort = %u\n", sockets[index]->local_port);
    printf("NEW_REQUESTER: Socket RemotePort = %u\n", sockets[index]->remote_port);
    printf("NEW_REQUESTER: Socket IP = %s\n", sockets[index]->remote_IP);

    printf("NEW_REQUESTER: Achou número de porta %u disponível.\n", local_port);

    // Cria o pacote de requisição de conexão
    Package* req = make_pkg(0, local_port, port, SYN_FLAG, NULL);

    int tries = 0;
    while(tries < 15){
        // Envia a requisição
        send_segment((char*)req, sizeof(Package), ip, REAL_SENDER_PORT);
        printf("NEW_REQUESTER: Tentativa %i: Enviou a requisição para %s.\n", tries+1, ip);
        // Aguarda chegar a resposta com timeout de 2 segundos
        timeout_requisition = FALSE;
        signal(SIGALRM, set_timeout_requisition);
        alarm(2);
        while(pkg_queue_isempty(sockets[index]->pkg_queue) && !timeout_requisition);
       
        if(!timeout_requisition){
            // Pega esse pacote            
            pthread_mutex_lock(&sockets[index]->queue_lock);
            Package * pkg = pkg_queue_pop(sockets[index]->pkg_queue);
            pthread_mutex_unlock(&sockets[index]->queue_lock);
            if (!is_corrupted(pkg) && ack_flag(pkg)){
                printf("NEW_REQUESTER: Recebeu o SYNACK.\n");
                free(pkg);
                break;
            }else{
                free(pkg);
            }
        }
        printf("NEW_REQUESTER: Timeout\n");

        tries++;        
    }

    if(tries >= 15){
        delete_connection_socket(index);
        free(req);
        perror("NEW_REQUESTER: Could not connect to server.");
        return -1;
    }

    // Bota a tréplica na fila de send do socket
    char reply[DATA_SIZE];
    memset(reply, 0, DATA_SIZE);
    pthread_mutex_lock(&sockets[index]->data_queue_lock);
    data_queue_push(sockets[index]->data_queue, reply);
    pthread_mutex_unlock(&sockets[index]->data_queue_lock);

    // E começa a rodar o socket
    sockets[index]->sending = TRUE;
    printf("NEW_REQUESTER: Sending = TRUE.\n");
    pthread_create(&sockets[index]->snd_thread, NULL, sender_thread, (void*)sockets[index]);
    pthread_create(&sockets[index]->rcv_thread, NULL, receiver_thread, (void*)sockets[index]);


    return index;     
}


// Depende da criação (new)
int delete_listener_socket(int listener_id){
    // Erro se o id passado não for o id especial de listener,
    if(listener_id != LISTENER_ID){
        perror(ID_LISTENER_ERROR);
        return FALSE;
    }
    // Erro se o listener não estiver criado
    if(listener_socket == NULL){
        printf("DELETE_LISTENER: ");
        perror(NULL_ERROR);
        return FALSE;
    }
    ListenerSocket* listener = listener_socket;
    
    if(pthread_cancel(listener->thread) != 0){
        perror(THREAD_CANCEL_ERROR);
    } 
      
    pthread_join(listener->thread, NULL);
    pthread_mutex_destroy(&listener->queue_lock);
    clearQueue(listener->req_ip_queue);
    clearQueue(listener->pkg_ip_queue);
    pkg_clear_queue(listener->pkg_queue);
    pkg_clear_queue(listener->req_pkg_queue);
    free(listener);
    
    return TRUE;
}

int delete_connection_socket(int socket_id){
    ConnectionSocket* socket = sockets[socket_id];
    if(socket== NULL){
        printf("DELETE_CONNECTION: ");
        printf(NULL_ERROR);
        return FALSE;
    }
    pkg_clear_queue(socket->pkg_queue);
    pthread_mutex_destroy(&socket->queue_lock);
    free(socket);
    
    return TRUE;
}


int getacknum(Package* pkg){
    return pkg->seq_number;
}

void* sender_thread(void* args){
    ConnectionSocket* socket = (ConnectionSocket*) args;
    
    Package* window[WINDOW_SIZE];
    base = (USER_TYPE == CLIENT)? 1 : 2;
    nextseqnum = (USER_TYPE == CLIENT)? 1 : 2;

    int run_timer = FALSE;
    int time;
    int endpoint = TIME;
    clock_t initial = clock();
    clock_t difference;

    while(TRUE){
        if(socket->sending && !socket->receiving){
            if(run_timer){
                // printf("SENDER_THREAD: Atualiza timer.\n");
                difference = clock() - initial;
                time = difference * 1000 / CLOCKS_PER_SEC; 
            } 
            // else printf("SENDER_THREAD: Não atualiza timer.\n");                  
            
            // Se tem dados para enviar
            if(!data_queue_isempty(socket->data_queue) || nextseqnum == socket->end_of_message){
                printf("--------------------------------------------\n");
                printf("SENDER_THREAD: Tem dados para enviar.\n");
                printf("SENDER_THREAD: Base = %u\n", base);
                printf("SENDER_THREAD: Nextseqnum = %u\n", nextseqnum);

                if(nextseqnum < base+WINDOW_SIZE){
                    char* data_sent;
                    char data[DATA_SIZE];
                    uint8_t flags_sent;
                    if(nextseqnum == socket->end_of_message){
                        printf("SENDER_THREAD: É fim de mensagem.\n");
                        data_sent = NULL;
                        flags_sent = END_FLAG;
                    }
                    else{
                        // Retira os dados da pilha
                        pthread_mutex_lock(&socket->data_queue_lock);
                        data_queue_pop(socket->data_queue, data);
                        pthread_mutex_unlock(&socket->data_queue_lock);
                        data_sent = data;
                        flags_sent = 0;
                    }                   
               
                    // Cria pacote com numero de sequencia nextseqnum
                    // e com os dados retirados da pilha
                    printf("SENDER_THREAD: Cria pacote com nextseqnum = %u\n", nextseqnum);
                    window[nextseqnum%WINDOW_SIZE] = make_pkg(nextseqnum, socket->local_port, socket->remote_port, flags_sent, (uint8_t*)data_sent);

                    print_pack(window[nextseqnum%WINDOW_SIZE], "SENDER_THREAD");

                    // Envia pacote para a camada de rede
                    send_segment((char*)window[nextseqnum%WINDOW_SIZE], sizeof(Package), socket->remote_IP, REAL_SENDER_PORT);
                    printf("SENDER_THREAD: Enviou segmento.\n");
                    
                    // Se 
                    if(base == nextseqnum){
                        printf("SENDER_THREAD: Base = nextseqnum = %u.\n", base);
                        initial = clock();
                        time = 0;
                        run_timer = TRUE;                        
                        printf("SENDER_THREAD: Reiniciou timer.\n");              
                    }
                    // if(nextseqnum == 1) socket->sending = FALSE;
                    nextseqnum++;
                }
                printf("--------------------------------------------\n");
            }

            if(!pkg_queue_isempty(socket->pkg_queue)){
                printf("--------------------------------------------\n");
                printf("SENDER_THREAD: Tem dados para receber.\n");
                pthread_mutex_lock(&socket->queue_lock);
                Package* pkg = pkg_queue_pop(socket->pkg_queue);
                pthread_mutex_unlock(&socket->queue_lock);
                if (!is_corrupted(pkg) && ack_flag(pkg)){
                    printf("SENDER_THREAD: Recebeu Ack %u:\n", pkg->seq_number);
                    print_pack(pkg, "SENDER_THREAD");
                    for (int i = base; i <= getacknum(pkg); ++i)
                    {
                        if(i == socket->end_of_message){
                            socket->end_of_message = -1;
                            socket->sending = FALSE;
                        }
                        free(window[i%WINDOW_SIZE]);
                    }
                    printf("SENDER_THREAD: Desalocou pacotes de %u a %u.\n", base, getacknum(pkg));
                    base = getacknum(pkg) + 1;
                    printf("SENDER_THREAD: Base = %u\n", base);

                    if (base == nextseqnum){
                        printf("SENDER_THREAD: Base = %u = nextseqnum => Pára o timer\n", base);
                        time = 0;
                        run_timer = FALSE;
                    }else{
                        printf("SENDER_THREAD: Base = %u != %u = nextseqnum => Reinicia o timer\n", base, nextseqnum);
                        initial = clock();                        
                        time = 0;
                        run_timer = TRUE;              
                    }
                    if(getacknum(pkg) == 1){
                        socket->sending = FALSE;
                        printf("\n");
                    }
                }else{
                   if (is_corrupted(pkg)) printf("SENDER_THREAD: Está corrompido.\n");
                   if (!ack_flag(pkg)) printf("SENDER_THREAD: Não é um ack.\n");
                   printf("SENDER_THREAD: Ignora.\n");
                }
                free(pkg);
                printf("SENDER_THREAD: Desalocou pacote recebido.\n");
                printf("--------------------------------------------\n");
            }

            if (time >= endpoint){
                printf("--------------------------------------------\n");
                printf("SENDER_THREAD: Timeout!\n");                
                initial = clock();
                time = 0;
                run_timer = TRUE;
                printf("SENDER_THREAD: Reinicia o timer\n");
                printf("SENDER_THREAD: Reenvia janela:\n");
                for(int i = base; i < nextseqnum; i++){
                    printf("SENDER_THREAD: Pacote %u:\n", i);
                    print_pack(window[i%WINDOW_SIZE], "SENDER_THREAD");
                    send_segment((char*)window[i%WINDOW_SIZE], sizeof(Package), socket->remote_IP, REAL_SENDER_PORT);          
                    printf("SENDER_THREAD: Enviado.\n");
                }
                printf("--------------------------------------------\n");
            }
        }        
    }
}



int send_message(int socket_id, char* snd_data, int length){
    ConnectionSocket* socket = sockets[socket_id];
    if (socket == NULL){
        printf("SEND_MESSAGE: ");
        printf(NULL_ERROR);
        return -1;
    }
    while(socket->receiving == TRUE || socket->sending == TRUE);
    socket->sending = TRUE;

    printf("\n");
    printf("SEND_MESSAGE: Começa.\n");
    printf("SEND_MESSAGE: length = %i\n", length);
    for (int j = 0; j < length; ++j)
    {
        printf("%c", snd_data[j]);       
    }
    printf("\n");


    uint32_t initialseqnumber = (nextseqnum == 1)? 2: nextseqnum;
    printf("SEND_MESSAGE: initialseqnumber = %u\n", initialseqnumber);
    
    int pkg_amount = length / DATA_SIZE;
    if (length % DATA_SIZE != 0) pkg_amount++;

    // Indica qual o próximo num de sequencia,
    // após o último pacote da mensagem
    socket->end_of_message = initialseqnumber + pkg_amount;
    printf("SEND_MESSAGE: end_of_message = %u\n", socket->end_of_message);

    printf("SEND_MESSAGE: Enviar %u pacotes\n", pkg_amount);
    int i;
    for (i = 0; i < (length - (length % DATA_SIZE)); i+= DATA_SIZE){
        printf("SEND_MESSAGE: Coloca na fila de envio\n");
        printf("SEND_MESSAGE: Data = ");
        for (int j = i; j < DATA_SIZE; ++j)
        {
            if(33 <= snd_data[j] &&  snd_data[j] <= 126 && snd_data[j] != ' ')
                printf("%c ", snd_data[j]);
            else if(snd_data[j] == ' ')
                printf("[SPC] ");
            else
                printf("0x%x ", snd_data[j]);
        }
        printf("\n");

        pthread_mutex_lock(&socket->data_queue_lock);
        data_queue_push(socket->data_queue, snd_data+i);
        pthread_mutex_unlock(&socket->data_queue_lock);
    }

    // Envia o resto
    if(length % DATA_SIZE != 0){
        // printf("SEND_MESSAGE: Coloca na fila de envio\n");
        char tail [DATA_SIZE];
        int a = length / DATA_SIZE;
        printf("SEND_MESSAGE: Coloca na fila de envio\n");
        memcpy((void*)tail, (void*) (snd_data + (length - (length % DATA_SIZE))), length%DATA_SIZE);
        printf("SEND_MESSAGE: Data = ");
        for (int j = 0; j < length%DATA_SIZE; ++j)
        {
            if(33 <= tail[j] &&  tail[j] <= 126 && tail[j] != ' ')
                printf("%c ", tail[j]);
            else if(tail[j] == ' ')
                printf("[SPC] ");
            else
                printf("0x%x ", tail[j]);
        }
        printf("\n");

        pthread_mutex_lock(&socket->data_queue_lock);
        data_queue_push(socket->data_queue, tail);
        pthread_mutex_unlock(&socket->data_queue_lock);
    } 

    while (base < socket->end_of_message);

    printf("SEND_MESSAGE: Termina\n");
    return length;
}

// Funções do destinatário
uint32_t expectedseqnum = 1;
void* receiver_thread(void* args){
    expectedseqnum = (USER_TYPE == SERVER)? 1 : 2;
    
    ConnectionSocket* socket = (ConnectionSocket *) args;
    Package * ack;
    int was_end = FALSE;
    while(TRUE){
        if(socket->receiving && !socket->sending){
            // printf("RECEIVER_THREAD: É para ouvir.\n");
            if(!pkg_queue_isempty(socket->pkg_queue)){
                printf("--------------------------------------------\n");
                printf("RECEIVER_THREAD: Tem dados pra receber.\n");
                pthread_mutex_lock(&socket->queue_lock);
                Package * pkg = pkg_queue_pop(socket->pkg_queue);
                pthread_mutex_unlock(&socket->queue_lock);
                printf("RECEIVER_THREAD: Tira da fila.\n");
                
                if(!is_corrupted(pkg)){
                    if(pkg->seq_number == expectedseqnum){
                        printf("RECEIVER_THREAD: Numero de sequencia é o esperado:\n");
                        print_pack(pkg, "RECEIVER_THREAD");
                        if(end_flag(pkg)){
                            printf("RECEIVER_THREAD: É fim de mensagem.\n");                            
                            socket->end_of_message = expectedseqnum;
                            socket->receiving = FALSE;
                        }
                        else{
                            char data[DATA_SIZE];
                            memcpy((void*)data, (void*)pkg->data, DATA_SIZE);
                            
                            if(expectedseqnum != 1){
                                pthread_mutex_lock(&socket->data_queue_lock);
                                data_queue_push(socket->data_queue, data);
                                pthread_mutex_unlock(&socket->data_queue_lock);
                                printf("RECEIVER_THREAD: Passa para a camada superior.\n");
                            }
                            else{
                                socket->receiving = FALSE;
                            }
                        }                 

                        printf("--------------------------------------------\n"); 

                        free(ack);
                        ack = make_pkg(expectedseqnum, socket->local_port, socket->remote_port, ACK_FLAG, NULL);

                        printf("--------------------------------------------\n");
                        printf("RECEIVER_THREAD: Criou o ACK:.\n");
                        print_pack(ack, "RECEIVER_THREAD");
                        
                        
                        send_segment((char*)ack, sizeof(Package),socket->remote_IP, REAL_SENDER_PORT);

                        printf("RECEIVER_THREAD: Enviou o ack.\n");
                        printf("--------------------------------------------\n");
                        
                        // if(expectedseqnum == 1) socket->receiving = FALSE;
                        expectedseqnum++;                       
                    }
                    else{
                        printf("RECEIVER_THREAD: Número de sequência NÃO é o esperado.\n");
                        printf("RECEIVER_THREAD: Expected = %u\n", expectedseqnum);
                        printf("RECEIVER_THREAD: SeqNumber = %u\n", pkg->seq_number);
                        printf("RECEIVER_THREAD: Envia ACK %u\n", ack->seq_number);
                        printf("--------------------------------------------\n");
                        send_segment((char*)ack, sizeof(Package),socket->remote_IP, REAL_SENDER_PORT);
                    }
                }
                else{
                    printf("RECEIVER_THREAD: Está corrompido.\n");
                    printf("RECEIVER_THREAD: Envia ACK %u\n", ack->seq_number);
                    printf("--------------------------------------------\n");
                    send_segment((char*)ack, sizeof(Package),socket->remote_IP, REAL_SENDER_PORT);
                }
                free(pkg);
                printf("RECEIVER_THREAD: Desalocou o pacote recebido.\n");
            }  
        }          
    }
}


volatile int recv_timeout = FALSE;
void set_recv_timeout(int sig){
    recv_timeout = TRUE;
}

int receive_message(int socket_id, char* rcv_data, int length){
    ConnectionSocket* socket = sockets[socket_id];
    if (socket == NULL){
        printf("RECEIVE_MESSAGE: ");
        printf(NULL_ERROR);
        return -1;
    }
    while(socket->sending == TRUE || socket->receiving == TRUE);
    socket->receiving = TRUE;

    printf("\n");
    printf("RECEIVE_MESSAGE: Começa.\n");
    
    uint32_t initial_expected = expectedseqnum;
    if(initial_expected == 1) initial_expected = 2;

    int pkg_amount = length / DATA_SIZE;
    if (length % DATA_SIZE != 0) pkg_amount++;

    socket->end_of_message = -1;

    printf("RECEIVE_MESSAGE: initial_expected = %u\n", initial_expected);
    printf("RECEIVE_MESSAGE: pkg_amount = %i\n", pkg_amount);

    int received_data = 0;
    signal(SIGALRM, set_recv_timeout);
    while(received_data < length){
        char data[DATA_SIZE]; 

        if(recv_timeout) break;      
        while(data_queue_isempty(socket->data_queue) || socket->end_of_message != -1);
        
        if(socket->end_of_message != -1){
            printf("RECEIVE_MESSAGE: Fim da mensagem.\n");
            socket->end_of_message = -1;         
            break;
        }
        else{
            printf("RECEIVE_MESSAGE: Preenche dados.\n");
            pthread_mutex_lock(&socket->data_queue_lock);
            data_queue_pop(socket->data_queue, data); 
            pthread_mutex_unlock(&socket->data_queue_lock);
            memcpy((void*)rcv_data + received_data, (void*)data, DATA_SIZE);
            received_data += DATA_SIZE;            
            alarm(4);
        }       
    }


    
    printf("RECEIVE_MESSAGE: Termina.\n");
    return received_data;
}
