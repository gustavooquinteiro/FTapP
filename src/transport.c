#include "../include/transport.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Struct do socket da camada de transporte
struct tcp_socket{
    int id;
};

// Nome simplificado para struct de endereço do socket
typedef struct sockaddr_in SockAddrIn;
typedef struct sockaddr SockAddr;

tcp_socket* new_connection_socket(tcp_socket* listener){
    tcp_socket* sock = (tcp_socket*) malloc(sizeof(tcp_socket));

    SockAddrIn client_address;
    int addr_len = sizeof(client_address);

    sock->id = accept(listener->id, (SockAddr*)&client_address, (socklen_t*)&addr_len);
    if(sock->id == -1) return NULL;
    return sock;
}

tcp_socket* new_listener_socket(int port){
    tcp_socket* sock = (tcp_socket*) malloc(sizeof(tcp_socket));

    // Cria o socket
    sock->id = socket(AF_INET, SOCK_STREAM, PROTOCOL_VALUE);
    if(sock->id == -1){
        return NULL;
    }
    // Seta um endereço de socket para 'adress'
    SockAddrIn server_address;
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);
    
    if(bind(sock->id, (SockAddr*)&server_address, sizeof(server_address)) == -1){
        return NULL;
    }
    
    if(listen(sock->id, LISTEN_QUEUE) == -1){
        return NULL;
    }   

    return sock;
}

tcp_socket* new_requester_socket(int port, char* address){
    tcp_socket* sock = (tcp_socket*) malloc(sizeof(tcp_socket));

    // Cria o socket
    sock->id = socket(AF_INET, SOCK_STREAM, PROTOCOL_VALUE);
    if(sock->id == -1){
        return NULL;
    }
    
    // Seta um endereço de socket para 'adress' na porta 'port'
    SockAddrIn server_address;
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = inet_addr(address); 
    server_address.sin_port = htons(port);

    // Envia requisição de conexão
    if(connect(sock->id, (SockAddr*)&server_address, sizeof(server_address)) == -1){
        return NULL;    
    }

    return sock;
}

int send_message(tcp_socket* sock, void* snd_data, unsigned int length){
    int size = send(sock->id, snd_data, length, SEND_FLAGS);
    return size;
}


int recieve_message(tcp_socket* sock, void* rcv_data, unsigned int length, int flag){
    int size;
    if(flag == 0)
        size = recv(sock->id, rcv_data, length, 0);
    else 
        size = recv(sock->id, rcv_data, length, MSG_WAITALL);
    
    return size;
}

int delete_tcp_socket(tcp_socket* sock){
    int value = shutdown(sock->id, SHUT_RDWR);
    free(sock);
    return value;
}

int get_peer_ip(tcp_socket* sock, char* ip){
    SockAddrIn address;
    socklen_t length = sizeof(address);
    getpeername(sock->id, (SockAddr*)&address, &length);
    
    strcpy(ip, inet_ntoa(address.sin_addr));
    return 0;
}
