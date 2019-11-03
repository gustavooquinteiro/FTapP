#include "../include/client.h"

#include "../include/transport.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> 

#define DATA_PORT 8074
#define CONTROL_PORT 8090

int min(int a, int b);
void toBytes(uint8_t* bytes, uint64_t integer);
uint64_t toLong(uint8_t* bytes);
uint32_t toInt(uint8_t* bytes);

uint64_t get_filesize(FILE* file){
	uint64_t previous = ftell(file);

	fseek(file, 0, SEEK_END);
	uint64_t size = ftell(file);

	fseek(file, 0, previous);

	return size;
}

int send_file(char* file_name, char* ip_address)
{
	uint32_t PKG_SIZE = 1000;

    printf("send_package client.c\n");
    printf("%s\n", file_name);

    FILE* myfile = fopen(file_name, "rb"); 

    // Cria socket de conexão
    tcp_socket* socket = new_requester_socket(CONTROL_PORT, ip_address);
    if(socket == NULL) return -1;

    // Envia requisição para conexão
    char request_msg[1] = {'A'};
    if(send_message(socket, request_msg, 1) == -1) return -1;

    // Recebe resposta do servidor (maximo de bytes no pacote)
    uint8_t max_pkg_bytes[4];
    if(recieve_message(socket, max_pkg_bytes, 4, 0) == -1) return -1;
    uint32_t max_pkg = toInt(max_pkg_bytes);

    // Calcula tamanho do arquivo  
    uint64_t filesize = get_filesize(myfile);

    // Envia o tamanho do arquivo e tamanho de cada pacote
    uint32_t pkg_size = min(max_pkg, PKG_SIZE);    
    uint8_t sizes_bytes[8 + 4];
    toBytes(sizes_bytes, filesize);
    toBytes(sizes_bytes + 8, pkg_size);
    if(send_message(socket, sizes_bytes, 8 + 4) == -1) return -1;
    printf("Tamanho '%lu' enviado.\n", filesize);


    // Envia arquivo em pacotes de pkg_size bytes
    uint64_t rest = filesize; 
    uint8_t buffer[pkg_size];
    int count = 1;
    while(rest > 0){
        int size = min(rest, pkg_size);
        fread(buffer, sizeof(char), size, myfile);

        int bytes_sent = send_message(socket, buffer, size);
        if(bytes_sent == -1){
            exit(EXIT_FAILURE);        
        }
        printf("Pacote %i com %i bytes enviado.\n", count, bytes_sent);

        rest -= bytes_sent; 
        count++;     
    }  
    fclose(myfile);
    printf("Arquivo enviado\n");
    
    // Recebe mensagem de confirmação
    char server_msg[50];
    int server_msg_size = recieve_message(socket, server_msg, PKG_SIZE, 0);
    if(server_msg_size == -1) return -1;
    server_msg[server_msg_size] = '\0';
    printf("Mensagem do servidor: %s\n", server_msg);

    delete_tcp_socket(socket);
}


int min(int a, int b){
    if(b <= a) return b;
    else return a;
}

void toBytes(uint8_t* S, uint64_t L)
{
    for (int i = 0; i < 8; ++i)
    {
        S[i] = L >> (8*(7-i));
    }
}

uint64_t toLong(uint8_t* S){
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
    {
        value = value << 8;
        value = value | S[i];
    }
    return value;
}

uint32_t toInt(uint8_t* S){
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i)
    {
        value = value << 8;
        value = value | S[i];
    }
    return value;
}