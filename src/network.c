#include "../include/network.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXBUFF 100
#define TRUE 1
#define FALSE 0
#define SND_PORT 8075
#define RCV_PORT 8075

typedef struct sockaddr_in SockAddrIn;
typedef struct sockaddr SockAddr;

#define CREATION_FAIL   "RECV_SEGMENT: Socket creation failed"
#define BIND_FAIL       "RECV_SEGMENT: Socket bind failed"
#define RECV_FAIL       "RECV_SEGMENT: Socket recieve failed"
#define SEND_FAIL       "RECV_SEGMENT: Socket send failed"


int send_segment(char* buffer, int buffsize, const char* IP, int real_port){
	// Criando o socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  
    if (sockfd == -1) {
        perror(CREATION_FAIL); 
        return FALSE; 
    }

    // Filling server information 
    SockAddrIn servaddr;  
    memset(&servaddr, 0, sizeof(servaddr));    
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(real_port); 
    servaddr.sin_addr.s_addr = inet_addr(IP);
    
    // printf("SEND_SEGMENT: Criou o socket.\n"); 
    
    // Envia os dados
    int result = sendto(sockfd, buffer, buffsize, MSG_CONFIRM, (SockAddr*) &servaddr, sizeof(servaddr));
    if(result == -1){
    	perror(SEND_FAIL); 
        return FALSE; 
    }

    // printf("SEND_SEGMENT: Enviou o segmento (%i bytes enviados).\n", result);
  	
  	// Fecha o socket
    close(sockfd);

    // printf("SEND_SEGMENT: Fechou o socket.\n");
    
    return TRUE; 
}

int recv_segment(char* buffer, int buffsize, char* IP, int real_port){
	int result;    
      
    // Creating socket file descriptor
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) { 
        perror(CREATION_FAIL);
        return FALSE;
    } 
    int truth = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&truth,sizeof(int));
      
    // Filling server information
    SockAddrIn servaddr; 
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(real_port);     
    // printf("RECV_SEGMENT: Criou o socket.\n");

    // Bind the socket with the server address 
    result = bind(sockfd, (SockAddr*)&servaddr,sizeof(servaddr));
    if (result == -1) 
    { 
        perror(BIND_FAIL);
        close(sockfd);
        return FALSE; 
    } 
    // printf("RECV_SEGMENT: Vinculou o socket.\n");    
    
    // Recebe os dados
	SockAddrIn cliaddr;
	int len = sizeof(cliaddr);
    result = recvfrom(sockfd, buffer, buffsize, MSG_WAITALL, (SockAddr*) &cliaddr, (socklen_t*) &len);
    if(result == -1){
    	perror(RECV_FAIL); 
        close(sockfd);
        return FALSE;
    }
    // printf("RECV_SEGMENT: Recebeu os dados (%u bytes recebidos).\n", result);  

    strcpy(IP, inet_ntoa(cliaddr.sin_addr));

    // printf("RECV_SEGMENT: Ip do cliente é %s\n", inet_ntoa(cliaddr.sin_addr));  

	// Fecha o socket
    close(sockfd);

    // printf("RECV_SEGMENT: Fechou o socket.\n");  
      
    return TRUE; 
}

