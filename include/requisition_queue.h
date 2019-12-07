#ifndef _QUEUE_H
#define _QUEUE_H

// Assinaturas das funções e struct
typedef struct req_queue RequisitionQueue;
typedef struct segment Segment;

// Função que aloca na memoria uma queue
RequisitionQueue * new_req_queue();

// Função que insere um novo elemento na queue.
void req_queue_push(RequisitionQueue* q, Segment* segment, char* ip);

// Função que remove o elemento da frente da queue 
Segment* req_queue_pop(RequisitionQueue* q, char * returned_ip);

// Função que verifica se a queue está vazia
int req_queue_isempty(RequisitionQueue* q);

// Função que retorna o elemento da frente da queue
Segment* req_queue_front(RequisitionQueue* q);

// Função que desaloca a queue da memória
void req_clear_queue(RequisitionQueue * queue);

#endif
