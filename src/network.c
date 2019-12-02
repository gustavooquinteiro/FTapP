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
#define PORT 8074

typedef struct sockaddr_in SockAddrIn;
typedef struct sockaddr SockAddr;

#define CREATION_FAIL   "Socket creation failed"
#define BIND_FAIL       "Socket bind failed"
#define RECV_FAIL       "Socket recieve failed"
#define SEND_FAIL       "Socket send failed"


int send_segment(char* buffer, int buffsize, char* IP){
	// Criando o socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  
    if (sockfd == -1) {
        perror("socket creation failed"); 
        return FALSE; 
    }

    // Filling server information 
    SockAddrIn servaddr;  
    memset(&servaddr, 0, sizeof(servaddr));    
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = inet_addr(IP);
     
    // Envia os dados
    int result = sendto(sockfd, buffer, buffsize, MSG_CONFIRM, (SockAddr*) &servaddr, sizeof(servaddr));
    if(result == -1){
    	perror(SEND_FAIL); 
        return FALSE; 
    }
  	
  	// Fecha o socket
    close(sockfd);
    
    return TRUE; 
}

int recv_segment(char* buffer, int buffsize, char* IP){
	int result;    
      
    // Creating socket file descriptor
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) { 
        perror(CREATION_FAIL);
        return FALSE;
    }     
      
    // Filling server information
    SockAddrIn servaddr; 
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    result = bind(sockfd, (SockAddr*)&servaddr,sizeof(servaddr));
    if (result == -1) 
    { 
        perror(BIND_FAIL); 
        return FALSE; 
    } 
    
    // Recebe os dados
	SockAddrIn cliaddr;
	int len;    
    result = recvfrom(sockfd, buffer, buffsize, MSG_WAITALL, (SockAddr*) &cliaddr, &len);
    if(result == -1){
    	perror(RECV_FAIL); 
        return FALSE;
    }

    strcpy(IP, inet_ntoa(cliaddr.sin_addr));

	// Fecha o socket
    close(sockfd);
      
    return TRUE; 
}

