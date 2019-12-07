#include "../include/ip_queue.h"
#include <stdlib.h>
#include <string.h>


#define SIZE 16

typedef struct node node;


struct node
{
	char ip[SIZE];
	node* next;
	node* prev;
};

struct ip_queue{
	node* 	back;
	node* 	front;
};


IpQueue* new_ip_queue(){
	IpQueue* new = (IpQueue*) malloc(sizeof(IpQueue));
	new->back = NULL;
	new->front = NULL;
    return new;
}

void ip_queue_push(IpQueue* q, char* ip){
	node* inserted = (node*) malloc(sizeof(node));
	memcpy((void*)inserted->ip, (void*)ip, SIZE);

	if(q->back != NULL){
		q->back->prev = inserted;
	}else{
		q->front = inserted;
	}

	inserted->next = q->back;
	inserted->prev = NULL;
	q->back = inserted;
}

void ip_queue_pop(IpQueue* q, char* return_ip){
	node* removed = q->front;

	if(removed->prev != NULL){
		removed->prev->next = NULL;
	}else{
		q->back = NULL;
	}

	q->front = removed->prev;

	if(return_ip != NULL) 
		memcpy((void*)return_ip, (void*)removed->ip, SIZE);
	
	free(removed);
}

int ip_queue_isempty(IpQueue* q){
	return (q->front == NULL);
}

void ip_clear_queue(IpQueue * queue){
    if (queue){
    	while(!ip_queue_isempty(queue)){
    		ip_queue_pop(queue, NULL);
    	}
        free(queue);
    }
}

