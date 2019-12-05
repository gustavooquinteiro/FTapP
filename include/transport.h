#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include <stdint.h>

#define CLIENT 1
#define SERVER 0

typedef struct connection_socket ConnectionSocket;
typedef struct listener_socket ListenerSocket;
typedef struct package Package;

int transport_init();

int new_listener_socket(int port);
int new_connection_socket(int listener);
int new_requester_socket(uint16_t port, const char* ip);

int delete_listener_socket(int listener);
int delete_connection_socket(int socket);

int get_peer_ip(int socket, char* ip);
int send_message(int socket, char* snd_data, int length);
int receive_message(int socket, char* rcv_data, int length);


#endif
