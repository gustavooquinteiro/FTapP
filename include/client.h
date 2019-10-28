#ifndef _CLIENT_H
#define _CLIENT_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

#define PROTOCOL_VALUE 0
#define PORT 80
#define ADDRESS "127.0.0.1"
#define SOCKET_FAILED_EXCEPTION "Socket creation failed"
#define SERVER_FAILED_EXCEPTION "Connection with server failed"


int create_connection();
struct sockaddr_in connect_with_server(int socket, int port, char * address);
int send_package();
int main(int argc, char **argv);

#endif
