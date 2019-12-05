#include "../include/pkg_queue.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct node node;

struct node
{
	Package* pkg;
	node* next;
	node* prev;
};

struct pkg_queue{
	node* 	back;
	node* 	front;
};


PackageQueue* new_pkg_queue(){
	PackageQueue* new = (PackageQueue*) malloc(sizeof(PackageQueue));
	new->back = NULL;
	new->front = NULL;
    return new;
}

void pkg_queue_push(PackageQueue* q, Package* pkg){
	node* inserted = (node*) malloc(sizeof(node));
	inserted->pkg = pkg;

	if(q->back != NULL){
		q->back->prev = inserted;
	}else{
		q->front = inserted;
	}

	inserted->next = q->back;
	inserted->prev = NULL;
	q->back = inserted;
}

Package* pkg_queue_pop(PackageQueue* q){
	// printf("PKG_QUEUE_POP: Entrou aqui.\n");
	node* removed = q->front;

	if(removed->prev != NULL){
		removed->prev->next = NULL;
	}else{
		q->back = NULL;
	}

	q->front = removed->prev;
	Package* pkg = removed->pkg;
	free(removed);

	// printf("PKG_QUEUE_POP: Fez as operações.\n");

	return pkg;
}

Package* pkg_queue_front(PackageQueue* q){
	return q->front->pkg;
}


int pkg_queue_isempty(PackageQueue* q){
	return (q->front == NULL);
}

void pkg_clear_queue(PackageQueue * queue){
    if (queue){
    	while(!pkg_queue_isempty(queue)){
    		pkg_queue_pop(queue);
    	}
        free(queue);
    }
}

