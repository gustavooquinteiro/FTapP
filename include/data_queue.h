#ifndef _DATA_QUEUE_H
#define _DATA_QUEUE_H

typedef struct data_queue DataQueue;

DataQueue* new_data_queue();

void data_queue_push(DataQueue*, char*);

void data_queue_pop(DataQueue*, char*);

int data_queue_isempty(DataQueue*);

void data_clear_queue(DataQueue * queue);


#endif
