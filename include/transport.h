#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#define CLIENT 1
#define SERVER 0

int GBN_transport_init(int user_type);

int GBN_listen(int port);

int GBN_accept(int listener);

int GBN_connect(int server_port, const char* server_ip);

int GBN_send(int socket, char* snd_data, int length);

int GBN_receive(int socket, char* rcv_data, int length);

int GBN_peer_ip(int socket, char* ip);

void GBN_close(int socket);

void GBN_transport_end();

#endif