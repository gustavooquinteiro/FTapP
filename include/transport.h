#ifndef _TRANSPORT_H
#define _TRANSPORT_H

typedef struct connection_socket ConnectionSocket;
typedef struct listener_socket ListenerSocket;
typedef struct package Package;
// //

int transport_init();

ListenerSocket* new_listener_socket(int port);
ConnectionSocket* new_connection_socket(ListenerSocket*);
ConnectionSocket* new_requester_socket(int port, const char* ip);

void delete_listener_socket(ListenerSocket* listener);
void delete_connection_socket(ConnectionSocket* socket);

int get_peer_ip(ConnectionSocket* socket, char* ip);
int send_message(ConnectionSocket* socket, char* snd_data, int length);
int receive_message();
// // 
// tcp_socket* new_connection_socket(tcp_socket* listener);


// // Envia uma mensagem através do socket dado
// // retorna tamanho da mensagem enviada, ou EXIT_FAILURE cao haja erro
// int send_message(tcp_socket*, void* snd_data, unsigned int length);

// // Recebe uma mensagem através do socket dado
// // retorna tamanho da mensagem recebida ou EXIT_FAILURE cao haja erro
// int recieve_message(tcp_socket*, void* rcv_data, unsigned int length, int flag);

// // Exclui o socket passado
// int delete_tcp_socket(tcp_socket*);

// // Retorna o ip do par conectado no socket dado 
// int get_peer_ip(tcp_socket*, char* ip);

#endif
