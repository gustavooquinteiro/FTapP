#include "../include/data_queue.h"
#include <stdlib.h>
#include <string.h>


#define SIZE 15

typedef struct node node;


struct node
{
	char data[SIZE];
	node* next;
	node* prev;
};

struct data_queue{
	node* 	back;
	node* 	front;
};


DataQueue* new_data_queue(){
	DataQueue* new = (DataQueue*) malloc(sizeof(DataQueue));
	new->back = NULL;
	new->front = NULL;
    return new;
}

void data_queue_push(DataQueue* q, char* data){
	node* inserted = (node*) malloc(sizeof(node));
	memcpy((void*)inserted->data, (void*)data, SIZE);

	if(q->back != NULL){
		q->back->prev = inserted;
	}else{
		q->front = inserted;
	}

	inserted->next = q->back;
	inserted->prev = NULL;
	q->back = inserted;
}

void data_queue_pop(DataQueue* q, char* return_data){
	node* removed = q->front;

	if(removed->prev != NULL){
		removed->prev->next = NULL;
	}else{
		q->back = NULL;
	}

	q->front = removed->prev;

	if(return_data != NULL) 
		memcpy((void*)return_data, (void*)removed->data, SIZE);
	
	free(removed);
}

int data_queue_isempty(DataQueue* q){
	return (q->front == NULL);
}

void data_clear_queue(DataQueue * queue){
    if (queue){
    	while(!data_queue_isempty(queue)){
    		data_queue_pop(queue, NULL);
    	}
        free(queue);
    }
}

