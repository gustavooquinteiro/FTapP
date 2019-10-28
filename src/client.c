#include "../include/client.h"


int create_connection()
{
    return socket(AF_INET, SOCK_STREAM, PROTOCOL_VALUE);
}

struct sockaddr_in connect_with_server(int socket, int port, char * address)
{
    if (!create_connection())
    {
        perror(SOCKET_FAILED_EXCEPTION);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(address);
    server_address.sin_port = htons(port);

    if(connect(socket, (struct sockaddr *)&server_address, sizeof(server_address)))
    {
        perror(SERVER_FAILED_EXCEPTION);
        exit(EXIT_FAILURE);
    }

    else
    printf("connected to the server..\n");

    return server_address;
}


int send_package(FILE * arquivo)
{
    return submit_package(arquivo);
}

int main(int argc, char **argv)
{
    return app_start();
}
