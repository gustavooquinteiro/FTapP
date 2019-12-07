#include "../include/requisition_queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SIZE 16

typedef struct node node;

struct node
{
    char ip[16];
	Segment* segment;
	node* next;
	node* prev;
};

struct req_queue{
	node* 	back;
	node* 	front;
};


RequisitionQueue* new_req_queue(){
	RequisitionQueue* new = (RequisitionQueue*) malloc(sizeof(RequisitionQueue));
	new->back = NULL;
	new->front = NULL;
    return new;
}

void req_queue_push(RequisitionQueue* q, Segment* segment, char* ip){
	if (ip == NULL){
        return;
	}
	
	node* inserted = (node*) malloc(sizeof(node));
	inserted->segment = segment;
	
	strcpy(inserted->ip, ip);
	
	if(q->back != NULL){
		q->back->prev = inserted;
	}else{
		q->front = inserted;
	}

	inserted->next = q->back;
	inserted->prev = NULL;
	q->back = inserted;
}

Segment* req_queue_pop(RequisitionQueue* q, char * returned_ip){
	// printf("BLZ3\n");
	node* removed = q->front;

	if(removed->prev != NULL){
		removed->prev->next = NULL;
	}else{
		q->back = NULL;
	}

	// printf("BLZ1\n");

	q->front = removed->prev;
	Segment* segment = removed->segment;
	
	if(returned_ip != NULL) 
		strcpy(returned_ip, removed->ip);
	
	free(removed);

	// printf("BLZ2\n");

	return segment;

	// return NULL;
}

Segment* req_queue_front(RequisitionQueue* q){
	return q->front->segment;
}


int req_queue_isempty(RequisitionQueue* q){
	return (q->front == NULL);
}

void req_clear_queue(RequisitionQueue * queue){
    if (queue){
    	while(!req_queue_isempty(queue)){
    		req_queue_pop(queue, NULL);
    	}
        free(queue);
    }
    queue = NULL;
}

