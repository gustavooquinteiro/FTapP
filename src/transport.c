#include "../include/transport.h"

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
    
    if (connect(socket, (struct sockaddr *)&server_address, sizeof(server_address)))
    { 
        perror(SERVER_FAILED_EXCEPTION); 
        exit(EXIT_FAILURE); 
    } 
    else
        printf("connected to the server..\n");
    
    return server_address;
}

int create_control_connection()
{
    // Substituir os (char *) pelo tipo pacote, assim que tivermos definido a struct pacote
    char * hello = "Hello";
    char buffer[PKT_SIZE] = {0};
    int control_socket = create_connection();
    struct sockaddr_in control_conn  = connect_with_server(control_socket, CONTROL_PORT, ADDRESS);
    send(control_socket, hello, strlen(hello), PROTOCOL_VALUE); 
    int valread = read(control_socket, buffer, PKT_SIZE); 
    // Definir ACCEPT como a mensagem de aceitação vinda do servidor
    if (strcmp(buffer, ACCEPT))
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

int submit_package(FILE * arquivo)
{
    if (create_control_connection())
    {
        int server_conn = create_connection();
        struct sockaddr_in server = connect_with_server(server_conn, DATA_PORT, ADDRESS);
        send(server_conn, arquivo, sizeof(arquivo), PROTOCOL_VALUE);
        return EXIT_SUCCESS;
    }
}
