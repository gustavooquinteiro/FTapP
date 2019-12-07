#include "../include/seg_queue.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct node node;

struct node
{
	Segment* seg;
	node* next;
	node* prev;
};

struct seg_queue{
	node* 	back;
	node* 	front;
};


SegmentQueue* new_seg_queue(){
	SegmentQueue* new = (SegmentQueue*) malloc(sizeof(SegmentQueue));
	new->back = NULL;
	new->front = NULL;
    return new;
}

void seg_queue_push(SegmentQueue* q, Segment* seg){
	node* inserted = (node*) malloc(sizeof(node));
	inserted->seg = seg;

	if(q->back != NULL){
		q->back->prev = inserted;
	}else{
		q->front = inserted;
	}

	inserted->next = q->back;
	inserted->prev = NULL;
	q->back = inserted;
}

Segment* seg_queue_pop(SegmentQueue* q){
	// printf("PKG_QUEUE_POP: Entrou aqui.\n");
	node* removed = q->front;

	if(removed->prev != NULL){
		removed->prev->next = NULL;
	}else{
		q->back = NULL;
	}

	q->front = removed->prev;
	Segment* seg = removed->seg;
	free(removed);

	// printf("PKG_QUEUE_POP: Fez as operações.\n");

	return seg;
}

Segment* seg_queue_front(SegmentQueue* q){
	return q->front->seg;
}


int seg_queue_isempty(SegmentQueue* q){
	return (q->front == NULL);
}

void seg_clear_queue(SegmentQueue * queue){
    if (queue){
    	while(!seg_queue_isempty(queue)){
    		seg_queue_pop(queue);
    	}
        free(queue);
    }
}

