#include "../include/client.h"

#include "../include/transport.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h> 
#include <stdio.h>
#include <time.h>

#define PORT 8074
#define CONTROL_PORT 8090
#define PKG_SIZE 1000000

typedef struct thread_data
{
    int socket;
    FILE* file;
} Data;


uint32_t pkg_size = 0;
uint64_t filesize = 0;

void print_time(){
    char buff[100];
    time_t now = time (0);
    strftime (buff, 100, "%Y-%m-%d %H:%M:%S.000", localtime (&now));
    printf ("[%s] ", buff);
}


int min(int a, int b);

void toBytes32(uint8_t* S, uint32_t integer);
void toBytes64(uint8_t* S, uint64_t integer);
uint64_t toLong(uint8_t* bytes);
uint32_t toInt(uint8_t* bytes);

uint64_t get_filesize(FILE* file);

int ip_is_valid(const char* ip);

int create_data_connection(Data args)
{
    int returned_value;
    int socket = args.socket;
    FILE* myfile = args.file;
        
    // Envia arquivo em pacotes de pkg_size bytes
    uint64_t rest = filesize; 
    uint8_t buffer[pkg_size];
    print_time();
    printf("Sending file...\n");
    clock_t start;
    start = clock();
    while(rest > 0){
        int size = min(rest, pkg_size);
        fread(buffer, sizeof(char), size, myfile);

        int bytes_sent = send_message(socket, buffer, size);
        if(bytes_sent == -1){ 
            delete_connection_socket(socket);
            fclose(myfile);
            print_time();
            perror("Error(Send file)");
            return SEND_FILE_ERROR;
        }      
        rest -= bytes_sent;
    }  
    printf("\n");
    fclose(myfile);
    print_time();
    printf("Success: File sent.\n");
    
    // Recebe mensagem de confirmação
    char server_msg[50];
    printf("Waiting confirmation...\n");
    returned_value = receive_message(socket, server_msg, PKG_SIZE);
    if(returned_value == -1 || returned_value == 0){
        delete_connection_socket(socket);
        perror("Warning(Server confirm)");
        return SERVER_CONFIRM_ERROR;
    }
    server_msg[returned_value] = '\0';    

    delete_connection_socket(socket);
    printf("Server confirmation received: %s\n", server_msg);
    return SUCCESS;
}
    
int create_control_connection(Data args)
{
    int returned_value;
    int socket = args.socket;
    FILE* myfile = args.file;
    
   // Cria socket de conexão
    printf("Creating socket...\n");
    if(socket == -1){
        fclose(myfile);
        perror("Error(Socket creation)");
        return CONN_SOCKET_CREATION_ERROR;
    }
    printf("Success: Socket creation.\n");

    // Envia requisição para conexão
    printf("Sending request...\n");
    char request_msg[1] = {'A'};
    if(send_message(socket, request_msg, 1) == -1) {
        delete_connection_socket(socket);
        fclose(myfile);
        perror("Error(Request)");
        return REQUEST_ERROR;
    }
    printf("Success: Request sent.\n");

    // Recebe resposta do servidor (maximo de bytes no pacote)
    uint8_t max_pkg_bytes[4];
    printf("Waiting response...\n");
    returned_value = receive_message(socket, max_pkg_bytes, 4);
    if( returned_value == -1 || returned_value == 0){
        delete_connection_socket(socket);
        fclose(myfile);
        perror("Error(Response)");
        return RESPONSE_ERROR;
    }
    printf("Success: Response received.\n");

    uint32_t max_pkg = toInt(max_pkg_bytes);
    // Calcula tamanho do arquivo  
    filesize = get_filesize(myfile);

    // Envia o tamanho do arquivo e tamanho de cada pacote
    pkg_size = min(max_pkg, PKG_SIZE);    
    uint8_t sizes_bytes[8 + 4];
    toBytes64(sizes_bytes, filesize);
    toBytes32(sizes_bytes + 8, pkg_size);
    printf("Sending file info...\n");
    if(send_message(socket, sizes_bytes, 8 + 4) == -1){
        delete_connection_socket(socket);
        fclose(myfile);
        perror("Error(Send info)");
        return SEND_INFO_ERROR;
    }
    printf("Success: Info sent.\n"); 
    return SUCCESS;
}

int send_file(char* file_name, const char* ip_address)
{

    if(!ip_is_valid(ip_address)){
        printf("Error(IP): Bad address.\n");
        return INVALID_IP_ERROR;
    }
    printf("Success: Valid IP.\n");
    int returned_value;

    printf("Arquivo: %s\n",file_name);
    FILE* myfile = fopen(file_name, "rb"); 
    if (myfile == NULL){
        perror("Error(File)");
        return FILE_ERROR;
    }
    printf("Success: Open file.\n");
    

    int connection_socket = new_requester_socket(PORT, ip_address);

    Data data;
    data.socket = connection_socket;
    data.file = myfile;
    
    int result = create_control_connection(data);
    if(result == SUCCESS){
        return create_data_connection(data);
    }
    return result;
}


int min(int a, int b){
    if(b <= a) return b;
    else return a;
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

uint64_t get_filesize(FILE* file){
    uint64_t previous = ftell(file);

    fseek(file, 0, SEEK_END);
    uint64_t size = ftell(file);

    fseek(file, 0, previous);

    return size;
}

int ip_is_valid(const char* ip){
    int len = strlen(ip);
    if(len < 7 || len > 15) return 0;

    int dot_count = 0;
    int sym_count = 0;
    
    char symbol[4];
    for (int i = 0; i < len; ++i)
    {
        if(dot_count < 3 && ip[i] == '.' && sym_count >= 1){
            int numb = atoi(symbol);
            if(numb > 255) return 0;
            
            dot_count++;
            sym_count = 0;
        }
        else if(ip[i] >= '0' && ip[i] <= '9' && sym_count < 3){
            symbol[sym_count] = ip[i];
            symbol[sym_count+1] = '\0';
            sym_count++;            
        }
        else return 0;
    }
    int numb = atoi(symbol);
    if(numb > 255) return 0;
    
    return 1;
}
