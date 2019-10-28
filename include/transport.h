#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include <sys/socket.h>
#include <arpa/inet.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PROTOCOL_VALUE 0
#define DATA_PORT 80
#define CONTROL_PORT 81
#define PKT_SIZE 1024
#define ACCEPT "OK"
#define ADDRESS "127.0.0.1"
#define SOCKET_FAILED_EXCEPTION "Socket creation failed"
#define SERVER_FAILED_EXCEPTION "Connection with server failed"

int submit_package(FILE * arquivo);
int create_control_connection();
int create_connection();
struct sockaddr_in connect_with_server(int socket, int port, char * address);

#endif
