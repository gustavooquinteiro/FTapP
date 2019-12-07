#ifndef _NETWORK_H
#define _NETWORK_H

int recv_segment(char* buffer, int buffsize, char* IP, int real_port);
int send_segment(char* buffer, int buffsize, const char* IP, int real_port);

#endif