#ifndef _CLIENT_H
#define _CLIENT_H

#define SUCCESS 0
#define REQUEST_ERROR 2
#define RESPONSE_ERROR 4
#define SEND_INFO_ERROR 6
#define SERVER_CONFIRM_ERROR 8
#define CONN_SOCKET_CREATION_ERROR 10
#define FILE_ERROR 12
#define SEND_FILE_ERROR 14
#define INVALID_IP_ERROR 16

int send_file(char * file_name, const char* ip_address);

#endif
