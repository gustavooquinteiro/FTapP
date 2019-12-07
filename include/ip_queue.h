#ifndef _IP_QUEUE_H
#define _IP_QUEUE_H

typedef struct ip_queue IpQueue;

IpQueue* new_ip_queue();

void ip_queue_push(IpQueue*, char*);

void ip_queue_pop(IpQueue*, char*);

int ip_queue_isempty(IpQueue*);

void ip_clear_queue(IpQueue * queue);


#endif
