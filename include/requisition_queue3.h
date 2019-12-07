#ifndef _QUEUE_H
#define _QUEUE_H

// Definição de constantes para legibilizar código
#define ZERO 0
#define ONE 1
#define MALLOC_ERROR "Dynamic allocation failed"

// Assinaturas das funções e struct
typedef struct queue Queue;

// Função que aloca na memoria uma queue
Queue * defineQueue();

// Função que insere um novo elemento na queue.
Queue * qpush(Queue * queue, char * newRequisition);

// Função que remove o elemento da frente da queue 
Queue * qpop(Queue * queue);

// Função que verifica se a queue está vazia
int qisEmpty(Queue * queue);

// Função que retorna o elemento da frente da queue
char* qfront (Queue * queue);

// Função que retorna o elemento do fim da queue
char* qback(Queue * queue);

// Função que retorna o tamanho da queue
int qsize(Queue * queue);

// Função que desaloca a queue da memória
void clearQueue(Queue * queue);

#endif