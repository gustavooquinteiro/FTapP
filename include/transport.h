#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#define SEND_FLAGS 0
#define RECV_FLAGS MSG_WAITALL
#define PROTOCOL_VALUE 0
#define LISTEN_QUEUE 1
#define SOCKET_FAILED_EXCEPTION "Socket creation failed"
#define CONNECTION_FAILED_EXCEPTION "Socket connection failed"

typedef struct tcp_socket tcp_socket;

// 
tcp_socket* new_requester_socket(int port, char* address);

//
tcp_socket* new_listener_socket(int port);

// 
tcp_socket* new_connection_socket(tcp_socket* listener);


// Envia uma mensagem através do socket dado
// retorna tamanho da mensagem enviada, ou EXIT_FAILURE cao haja erro
int send_message(tcp_socket*, void* snd_data, unsigned int length);

// Recebe uma mensagem através do socket dado
// retorna tamanho da mensagem recebida ou EXIT_FAILURE cao haja erro
int recieve_message(tcp_socket*, void* rcv_data, unsigned int length, int flag);

// Exclui o socket passado
int delete_tcp_socket(tcp_socket*);

// Retorna o ip do par conectado no socket dado 
int get_peer_ip(tcp_socket*, char* ip);

#endif
