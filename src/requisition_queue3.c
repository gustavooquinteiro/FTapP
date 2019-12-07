#include "../include/requisition_queue.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>	

typedef struct queue{
	char   requisition[16]; 	
	struct queue * next;
	struct queue * begin;
	struct queue * end;
	int size;
} Queue; 

Queue * defineQueue(){
	Queue * queue = (Queue *)malloc(sizeof(Queue));
	if (!queue){
		perror(MALLOC_ERROR);
		exit(EXIT_FAILURE); 
	} else{
		queue->begin = NULL;
		queue->end = NULL;
		queue->next = NULL;
		queue->size = ZERO;
	}
	return queue;
}

Queue * qpush(Queue * queue, char* newReq){
	Queue * new = (Queue *)malloc(sizeof(Queue)); 
	if (!new){
		perror(MALLOC_ERROR); 
		exit(EXIT_FAILURE);
	} else{
		strcpy(new->requisition, newReq);
		new->next = NULL;
		if (qisEmpty(queue)){
			queue->begin = new;
		} else
			queue->end->next = new;
		queue->size += ONE;
		queue->end = new;
	}
	return queue; 
}

Queue * qpop(Queue * queue){
	if (qisEmpty(queue)){
		queue->end = NULL;
	} else{
		Queue *aux = queue->begin;
		queue->begin = aux->next; 		 
		free(aux);
	}
	queue->size -= ONE;
	return queue; 
}

int qisEmpty(Queue *queue){
	return (queue->begin == NULL)? ONE: ZERO; 
}

char* qfront (Queue * queue){
	return qisEmpty(queue)? NULL: queue->begin->requisition; 
}

char* qback (Queue * queue){
	return qisEmpty(queue)? NULL: queue->end->requisition;  
}

int qsize(Queue * queue){
	return queue->size;
}

void clearQueue(Queue * queue){
	if (queue)
		free(queue); 
}