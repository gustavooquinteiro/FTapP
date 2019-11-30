#ifndef _NETWORK_H
#define _NETWORK_H

int recv_segment(char* buffer, int buffsize, char* IP);
int send_segment(char* buffer, int buffsize, char* IP);

#endif