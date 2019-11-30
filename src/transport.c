#include "../include/transport.h"
#include "../include/network.h"
#include "../include/requisition_queue.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#define PKG_SIZE 20
#define MAX_PORTS 30
#define TRUE 1
#define FALSE 0

#define PORT_ERROR "Port unavailable."
#define PORT_LIMIT_ERROR "All ports are in use."
#define MUTEX_ERROR "Thread mutex init fail."
#define NULL_ERROR "Pointer is null."
#define THREAD_CANCEL_ERROR "Cancel thread error."

// Struct do connection socket da camada de transporte
struct connection_socket{
    int port;
    char destIP[16];  
};

// Struct do listener socket da camada de transporte
struct listener_socket{
    int port;
    Queue* req_queue;

    pthread_mutex_t lock;   
    pthread_t thread;   
};

// Struct do segmento da camada de transporte
typedef struct package{
    uint16_t    orig_port;
    uint16_t    dest_port;
    uint16_t    checksum;
    uint8_t     flags;
    uint8_t     data[13];
} Package;

uint8_t used_ports[MAX_PORTS] = {};

int next_port(){
    for (int i = 0; i < MAX_PORTS; ++i)
    {
        if(used_ports[i] == FALSE) return i;
    }
    return -1;
}

int is_corrupted(Package* pkg){
    uint32_t aux;
    uint16_t sum = 0;

    sum += pkg->orig_port;

    aux = (uint32_t)sum + (uint32_t)pkg->dest_port;
    if(aux >> 16 == 1){
        aux += 1;
    }
    sum = aux;

    aux = (uint32_t)sum + (uint32_t)pkg->checksum;
    if(aux >> 16 == 1){
        aux += 1;
    }
    sum = aux;


    uint16_t flags_and_data = pkg->flags;
    flags_and_data = flags_and_data << 8;
    flags_and_data |= pkg->data[0];

    aux = (uint32_t)sum + (uint32_t) flags_and_data;
    if(aux >> 16 == 1){
        aux += 1;
    }
    sum = aux;

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

void* listen_requisitions(void* data){
    ListenerSocket* listener = (ListenerSocket*) data; 
    int nada;   
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &nada);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &nada);

    while(TRUE){
        int result;
        char clientIP[16];
        Package pkg;
        result = recv_segment((char*)&pkg, sizeof(pkg), clientIP);

        // Se tiver corrompido, ignora o pacote
        // ou nao for uma tentativa de conexao,
        if(is_corrupted(&pkg) || !syn_flag(&pkg)) continue;

        pthread_mutex_lock(&listener->lock);
        // Caso contrário, coloca na fila
        qpush(listener->req_queue, clientIP);
        pthread_mutex_unlock(&listener->lock);
    }
}

ListenerSocket* new_listener_socket(int port){    
    if(used_ports[port] == TRUE){
        perror(PORT_ERROR);
        return NULL;
    }

    ListenerSocket* new = (ListenerSocket*) malloc(sizeof(ListenerSocket));

    used_ports[port] = TRUE;
    new->port = port;
    new->req_queue = defineQueue();

    // Cria mutex da thread para evitar erro
    // na manipulação da fila de requisições
    if (pthread_mutex_init(&new->lock, NULL) != 0)
    {
        printf(MUTEX_ERROR);
        free(new);
        return NULL;
    }

    // Cria thread para escutar requisições
    pthread_create(&new->thread, NULL, listen_requisitions, (void *)new);

    return new;
}

ConnectionSocket* new_connection_socket(ListenerSocket* listener){    
    if(listener == NULL){
        perror(NULL_ERROR);
        return NULL;
    }

    char ip[16];
    // Loop esperando a fila ter pelo menos uma requisição
    while(qisEmpty(listener->req_queue));

    strcpy(ip, qfront(listener->req_queue));
    pthread_mutex_lock(&listener->lock);
    
    qpop(listener->req_queue);    

    pthread_mutex_unlock(&listener->lock);
   

    // Se nao houver portas disponíveis, retorna nulo
    int port = next_port();
    if(port == -1){
        perror(PORT_LIMIT_ERROR);
        return NULL;
    }   

    // Caso contrário,
    // cria o socket de conexão, baseado no ip 
    // da próxima requisição de listener
    ConnectionSocket* new = (ConnectionSocket*) malloc(sizeof(ConnectionSocket));
    used_ports[port] = TRUE;
    new->port = port;
    strcpy(new->destIP, ip);

    return new;
}

ConnectionSocket* new_requester_socket(int port, const char* ip){ 
    // Se a porta requisitada nao estiver disponível, retorna nulo
    if(used_ports[port] == TRUE){
        perror(PORT_ERROR);
        return NULL;
    }

    // Caso contrário,
    // cria o socket de conexão, baseado na porta e no ip passado
    ConnectionSocket* new = (ConnectionSocket*) malloc(sizeof(ConnectionSocket));
    used_ports[port] = TRUE;
    new->port = port;
    strcpy(new->destIP, ip);

    return new;
}

void delete_listener_socket(ListenerSocket* listener){
    if(listener == NULL){
        perror(NULL_ERROR);
        return;
    }

    if(pthread_cancel(listener->thread) != 0){
        perror(THREAD_CANCEL_ERROR);
    } 
      
    pthread_join(listener->thread, NULL);
    pthread_mutex_destroy(&listener->lock);
    clearQueue(listener->req_queue);
    free(listener);
}

void delete_connection_socket(ConnectionSocket* socket){
    if(socket == NULL){
        perror(NULL_ERROR);
        return;
    }
    free(socket);
}

int get_peer_ip(ConnectionSocket* socket, char* ip){
    strcpy(ip, socket->destIP);
    return 1;
}

int send_message(ConnectionSocket* socket, char* snd_data, int length){
    
    return 1;
}

int receive_message(){ return 1;}




// tcp_socket* new_connection_socket(tcp_socket* listener){
//     tcp_socket* sock = (tcp_socket*) malloc(sizeof(tcp_socket));

//     SockAddrIn client_address;
//     int addr_len = sizeof(client_address);

//     sock->id = accept(listener->id, (SockAddr*)&client_address, (socklen_t*)&addr_len);
//     if(sock->id == -1) return NULL;
//     return sock;
// }




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

// int delete_tcp_socket(tcp_socket* sock){
//     int value = shutdown(sock->id, SHUT_RDWR);
//     free(sock);
//     return value;
// }









// // Nome simplificado para struct de endereço do socket
// typedef struct sockaddr_in SockAddrIn;
// typedef struct sockaddr SockAddr;

// tcp_socket* new_connection_socket(tcp_socket* listener){
//     tcp_socket* sock = (tcp_socket*) malloc(sizeof(tcp_socket));

//     SockAddrIn client_address;
//     int addr_len = sizeof(client_address);

//     sock->id = accept(listener->id, (SockAddr*)&client_address, (socklen_t*)&addr_len);
//     if(sock->id == -1) return NULL;
//     return sock;
// }



// // Thread de ouvir requisições
// void* listen_requisitions(void* data){
//     tcp_socket* sock = (tcp_socket*) data;    

//     while(TRUE){
//         SockAddrIn client_address;
//         int adress_len;
//         Package pkg;
//         int n = recvfrom(sock->id, (char*)pkg, PKG_SIZE,  
//                     RECV_FLAGS, (SockAddr*) &client_address, 
//                     &adress_len);

//         // Se tiver corrompido, ignora o pacote
//         if(is_corrupted(&pkg)) continue;

//         // Caso contrário
//     }
// }

// tcp_socket* new_listener_socket(int port){
//     tcp_socket* sock = (tcp_socket*) malloc(sizeof(tcp_socket));
//     SockAddrIn server_address;
    
//     // Cria o socket
//     sock->id = socket(AF_INET, SOCK_DGRAM, PROTOCOL_VALUE);
//     if (sock->id == -1) { 
//         return NULL;
//     }
//     memset(&server_address, 0, sizeof(server_address)); 
//     memset(&client_address, 0, sizeof(client_address));

//     // Seta a porta para o socket
//     server_address.sin_family = AF_INET; 
//     server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
//     server_address.sin_port = htons(port); 

//     // Associa o socket com o server address 
//     if(bind(sock->id, (SockAddr*)&server_address, sizeof(server_address)) == -1){
//         return NULL;
//     }

//     // Cria thread para escutar requisições
//     pthread_t thread;
//     pthread_create(&thread, NULL, listen_requisitions, (void *)sock);

//     return sock;
// }

// // tcp_socket* new_listener_socket(int port){
// //     tcp_socket* sock = (tcp_socket*) malloc(sizeof(tcp_socket));

// //     // Cria o socket
// //     sock->id = socket(AF_INET, SOCK_STREAM, PROTOCOL_VALUE);
// //     if(sock->id == -1){
// //         return NULL;
// //     }
// //     // Seta um endereço de socket para 'adress'
// //     SockAddrIn server_address;
// //     server_address.sin_family = AF_INET; 
// //     server_address.sin_addr.s_addr = htonl(INADDR_ANY);
// //     server_address.sin_port = htons(port);
    
// //     if(bind(sock->id, (SockAddr*)&server_address, sizeof(server_address)) == -1){
// //         return NULL;
// //     }
    
// //     if(listen(sock->id, LISTEN_QUEUE) == -1){
// //         return NULL;
// //     }
// //     return sock;
// // }

// tcp_socket* new_requester_socket(int port, const char* address){
//     tcp_socket* sock = (tcp_socket*) malloc(sizeof(tcp_socket));

//     // Cria o socket
//     sock->id = socket(AF_INET, SOCK_STREAM, PROTOCOL_VALUE);
//     if(sock->id == -1){
//         return NULL;
//     }
    
//     // Seta um endereço de socket para 'adress' na porta 'port'
//     SockAddrIn server_address;
//     server_address.sin_family = AF_INET; 
//     server_address.sin_addr.s_addr = inet_addr(address); 
//     server_address.sin_port = htons(port);

//     // Envia requisição de conexão
//     if(connect(sock->id, (SockAddr*)&server_address, sizeof(server_address)) == -1){
//         return NULL;    
//     }

//     return sock;
// }

