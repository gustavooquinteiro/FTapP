#include "../include/transport.h"
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define PORT            8074
#define CONTROL_PORT    8090
#define CTRLBUFF_SIZE   12
#define TRUE            1

#define LISTENER_CREATION_ERROR     "Listener creation error"
#define CONN_CREATION_ERROR         "Connection socket creation error"
#define REQUEST_ERROR               "Client request error"
#define RESPONSE_ERROR              "Server response error"
#define RECEIVE_INFO_ERROR          "Receive info error"
#define RECEIVE_FILE_ERROR          "Receive file error"
#define SERVER_CONFIRM_ERROR        "Server confirm error"


pthread_mutex_t lock;

void print_time(){
    char buff[100];
    time_t now = time (0);
    strftime (buff, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now));
    printf ("[%s] ", buff);
}

void toBytes32(uint8_t* S, uint32_t L)
{
    for (int i = 0; i < 4; ++i)
        S[i] = L >> (8*(3-i));
}

void toBytes64(uint8_t* S, uint64_t L)
{
    for (int i = 0; i < 8; ++i)
        S[i] = L >> (8*(7-i));
}

uint64_t toLong(uint8_t* S)
{
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
    {
        value = value << 8;
        value = value | S[i];
    }
    return value;
}

uint64_t toInt(uint8_t* S)
{
    uint64_t value = 0;
    for (int i = 0; i < 4; ++i)
    {
        value = value << 8;
        value = value | S[i];
    }
    return value;
}

uint32_t MAX_PKG_SIZE = 4000000;

void * receive_file(void * socket_connection)
{
    int returned_value;
    int* aux = (int*) socket_connection;
    int conn_socket = *aux;
    printf("hello from thread\n"); 
    char* hello = "Arquivo recebido com sucesso.";
    char control_buffer[CTRLBUFF_SIZE];
    // Envia o maximo de bytes num pacote
    uint8_t max_pkg_bytes[4];
    toBytes32(max_pkg_bytes, MAX_PKG_SIZE);
    printf("--------------------------------------\n");
    for (int i = 0; i < 4; ++i)
    {
        printf("0x%x ", max_pkg_bytes[i]);
    }
    printf("\n");
    printf("--------------------------------------\n");
    if(GBN_send(conn_socket, max_pkg_bytes, 4) == -1){
        perror(RESPONSE_ERROR);
        GBN_close(conn_socket);
        pthread_exit(NULL);
    }

    // Recebe o tamanho do arquivo e tamanho de cada pacote, determinado pelo cliente
    returned_value = GBN_receive(conn_socket, control_buffer, CTRLBUFF_SIZE);
    if(returned_value == -1 || returned_value == 0){        
        perror(RECEIVE_INFO_ERROR);
        GBN_close(conn_socket);
        pthread_exit(NULL);        
    }
    uint64_t filesize = toLong(control_buffer);
    uint32_t pkg_size = toInt(control_buffer+sizeof(uint64_t));
    
    char data_buffer[pkg_size];

    pthread_mutex_lock(&lock);

    char my_ip[20];
    GBN_peer_ip(conn_socket, my_ip);
    //printf("Conectado com %d\n", get_peer_ip(conn_socket)); 
    mkdir(my_ip, S_IRWXU);
    char file_name [300];
    strcpy(file_name, my_ip);
    strcat(file_name, "/");
    char name[20] = "cfile-\0";
    int index = 0;
    char convertion_buffer[300];
    while(TRUE){            
        char aux[300];
        strcpy(aux, file_name);
        strcat(aux, name);
        sprintf(convertion_buffer, "%i", index);
        strcat(aux, convertion_buffer);
        FILE * file = fopen(aux, "rb");
        if (file != NULL){
            index++;
            fclose(file);
        } else {
            break;
        }
    }
    strcat(file_name, name);
    strcat(file_name, convertion_buffer);
    printf("Name = %s\n", file_name);

    // Recebe o arquivo
    FILE* file = fopen(file_name, "wb");
    
    pthread_mutex_unlock(&lock);
    
    uint64_t sizeread = 0;
    printf("Filesize  = %lu\n", filesize);
    int count2 = 1;
    ssize_t msgsize;
    int64_t rest = filesize;
    int error = 0;
    print_time();
    printf("Receiving file...\n");
    while(rest > 0){
        int howMuchReceive = (rest < pkg_size) ? rest : pkg_size;
        msgsize = GBN_receive(conn_socket, data_buffer, howMuchReceive);

        if(msgsize == -1 || msgsize == 0) {
            error = 1;
            print_time();
            perror(RECEIVE_FILE_ERROR);
            break;
        }

        fwrite(data_buffer, sizeof(char), msgsize, file);        
        rest -= msgsize;

        // print_time();
        // printf("Recebidos %li bytes. Faltam %li bytes.\n", msgsize, rest);
    }   
    fclose(file);  
    if (error == 1) {
        int status = remove(file_name);
        GBN_close(conn_socket);
        pthread_exit(NULL);
    }

    print_time();
    printf("Success: File received.\n");
            
    // Envia resposta
    if(GBN_send(conn_socket, hello, 30) == -1){
        GBN_close(conn_socket);
        perror(SERVER_CONFIRM_ERROR);
        pthread_exit(NULL);
    }
    printf("Arquivo recebido com sucesso.\n");
    printf("end from thread\n"); 
    pthread_exit(NULL);
}


int main(int argc, char const *argv[])
{
    int returned_value;
    int attempts = 0;
    int listener_socket = -1;
    
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    
    GBN_transport_init(SERVER);
    while(listener_socket == -1 && attempts < 3){
        listener_socket = GBN_listen(PORT);
        attempts++;
    }
    if (listener_socket == -1){
        perror(LISTENER_CREATION_ERROR);
        exit(EXIT_FAILURE);
    }
    
    while(TRUE){
        int conn_socket = GBN_accept(listener_socket);
        if (conn_socket == -1){
            perror(CONN_CREATION_ERROR);
            continue;
        }

        printf("Fez o maluco\n");
        
        char connect_buffer[1];
        returned_value = GBN_receive(conn_socket, connect_buffer, 1);
        if(returned_value == -1 || returned_value == 0){        
            perror(REQUEST_ERROR);
            GBN_close(conn_socket);
            continue;
        }
        if(connect_buffer[0] != 'A'){
            printf("É diferente de 'A'\n");
            GBN_close(conn_socket);
            continue;
        }
        
        pthread_t thread;
        pthread_create(&thread, NULL, receive_file, (void*)&conn_socket);
        // printf("Criei a thread fodasse\n");
    }

    GBN_close(listener_socket);
    return 0;
}
