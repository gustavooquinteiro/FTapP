#include "../include/transport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

#define DATA_PORT 8074
#define CONTROL_PORT 8090
#define ERROR "Erro!"
#define CTRLBUFF_SIZE 12

uint32_t MAX_PKG_SIZE = 1000;

void toBytes32(uint8_t* S, uint32_t L)
{
    for (int i = 0; i < 4; ++i)
    {
        S[i] = L >> (8*(3-i));
    }
}

void toBytes64(uint8_t* S, uint64_t L)
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

uint64_t toInt(uint8_t* S){
    uint64_t value = 0;
    for (int i = 0; i < 4; ++i)
    {
        value = value << 8;
        value = value | S[i];
    }
    return value;
}

int main(int argc, char const *argv[])
{
    
    tcp_socket* listener_socket = new_listener_socket(CONTROL_PORT);
    

    while(1){
        char* hello = "Arquivo recebido com sucesso.";
        char control_buffer[CTRLBUFF_SIZE];

        tcp_socket* conn_socket = new_connection_socket(listener_socket);

        int meet_size = recieve_message(conn_socket, control_buffer, sizeof(control_buffer), 0);
        if(meet_size == -1){        
            perror(ERROR);
            continue;
        }
        if(meet_size == 0 || control_buffer[0] != 'A'){
            continue;
        }

        // get_peer_ip(conn_socket);
        // printf("Conectado com %d\n", get_peer_ip(conn_socket)); 


        // Envia o maximo de bytes num pacote
        uint8_t max_pkg_bytes[4];
        toBytes32(max_pkg_bytes, MAX_PKG_SIZE);
        if(send_message(conn_socket, max_pkg_bytes, sizeof(uint32_t)) == -1){
            perror(ERROR);
            continue;
        }

        // Recebe o tamanho do arquivo e tamanho de cada pacote, determinado pelo cliente
        uint64_t filesize;
        uint32_t pkg_size = MAX_PKG_SIZE;
        if(recieve_message(conn_socket, control_buffer, CTRLBUFF_SIZE, 0)!= -1){        
            filesize = toLong(control_buffer);
            // pkg_size = toInt(control_buffer+sizeof(uint64_t));
        }
        else{
            exit(EXIT_FAILURE);
        }



        char count[2] = "a\0";
        char data_buffer[pkg_size];

        char name[8] = "corno-\0";
        strcat(name, count);

        printf("Name = %s\n", name);

        // Recebe o arquivo
        FILE* corno = fopen(name, "wb");
        uint64_t sizeread = 0;
        printf("Filesize  = %lu\n", filesize);
        int count2 = 1;
        ssize_t msgsize;
        int64_t rest = filesize;
        while(rest > 0){
            int flag = (rest < pkg_size) ? 0 : 1;
            msgsize = recieve_message(conn_socket, data_buffer, pkg_size, flag);

            if(msgsize == -1) {
                perror(ERROR);
                exit(EXIT_FAILURE);
            }

            fwrite(data_buffer, sizeof(char), msgsize, corno);        
            rest -= msgsize;

            printf("Recebidos %li bytes. Faltam %li bytes.\n", msgsize, rest);
        }
        fclose(corno);
        
        // Envia resposta
        if(send_message(conn_socket, hello, 29) != -1){
            printf("Arquivo recebido com sucesso.\n");
        }
        else{
            exit(EXIT_FAILURE);
        }

        delete_tcp_socket(conn_socket);


        printf("%c\n", count[0]);
        count[0] = (char) count[0] + 1;
        printf("%c\n", count[0]);

    }

    
    delete_tcp_socket(listener_socket);
    return 0;
}
