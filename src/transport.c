#include "../include/transport.h"
#include "../include/requisition_queue.h"
#include "../include/data_queue.h"
#include "../include/seg_queue.h"
#include "../include/ip_queue.h"
#include "../include/timer.h"
#include "../include/network.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
// Para debug
#include <stdio.h>

#define TRUE  1
#define FALSE 0

#define MAX_SOCKETS 128
#define MAX_PORT    65535

#define WINDOW_SIZE 20
#define SEG_SIZE    26
#define DATA_SIZE   15

#define ACK_FLAG 0b00000100
#define SYN_FLAG 0b00000010
#define FIN_FLAG 0b00000001

#define ALLOC(TYPE) (TYPE*) malloc(sizeof(TYPE))
#define LOCK(MUTEX) pthread_mutex_lock(&MUTEX)
#define UNLOCK(MUTEX) pthread_mutex_unlock(&MUTEX)

typedef struct gbn_socket 	GBN_Socket;
typedef struct segment		Segment;
typedef struct requisition	Requisition;
typedef enum state			State;

// Estados possives
enum state{
	CLOSED,
	ESTABLISHED, 
	CLOSE_WAIT, 
	FIN_WAIT_1, 
	FIN_WAIT_2, 
	LAST_ACK, 
	TIME_WAIT, 
	SYN_SENT,
	SYN_SEND,
	SYN_ACK,
	FIN_SEND,
	SYN_RCVD,
	LISTEN,
	UNSET
};

struct gbn_socket
{
	// Propriedades fundamentais do socket
	int 		id;
	uint16_t 	local_port;
	uint16_t 	remote_port;
	char		remote_IP[16];
	Timer*		TTL;
	Timer*		close_timer;
	State  		state;

	// Filas/Buffers e os mutexes relacionados a eles
	RequisitionQueue*	req_queue;
	DataQueue*			snd_buffer;
	DataQueue*			rcv_buffer;
	pthread_mutex_t 	req_mutex;
	pthread_mutex_t 	snd_mutex;
	pthread_mutex_t 	rcv_mutex;
	
	// Variáveis do remetente
	uint32_t	nextseqnum;
	uint32_t	base;
	Segment*	window[WINDOW_SIZE];
	Timer*		gbn_timer;

	// Variáveis do destinatário
	uint32_t 	expectedseqnum;
	Segment*	ack;
};

struct segment
{
	uint32_t    seq_number;
    uint16_t    orig_port;
    uint16_t    dest_port; 
    uint16_t    checksum;   
    uint8_t     flags;
    uint8_t     data[DATA_SIZE];    
};

// VARIÁVEIS GLOBAIS
// Vetor que guarda os SOCKETS
GBN_Socket* SOCKETS[MAX_SOCKETS];

// Thread para ouvir a camada de rede
SegmentQueue* 	NETWORK_SEG_BUFFER;
IpQueue*		NETWORK_IP_BUFFER;
pthread_mutex_t NETWORK_MUTEX;
pthread_t 		NETWORK_THREAD;
int 			END_NETWORK = FALSE;

// Thread em que o kernel vai rodar
pthread_t 	KERNEL;
int 		END_KERNEL = FALSE;

int 		USER_TYPE;
uint16_t 	REAL_SENDER_PORT;
uint16_t 	REAL_RECEIVER_PORT;

pthread_mutex_t DELETE_MUTEX;


// UTILIZADO PELAS FUNÇÕES PUBLICAS
// Função associada à thread que ouve da camada de rede
void* listen_network_layer(void* data);
// Função associada à thread do kernel
void* kernel_thread(void* data);
// Aloca um socket de conexão
GBN_Socket* make_connection(int id, uint16_t local_port, uint16_t remote_port, const char* remote_IP);
// Aloca um socket ouvidor
GBN_Socket* make_listener(int id, uint16_t local_port);
// Desaloca um socket
void delete_socket(GBN_Socket* socket);
// Acha um id disponível
int get_next_id();
// Pega uma porta disponível
int get_port(int remote_port, const char* remote_ip);

// FUNÇÕES PUBLICAS
int GBN_transport_init(int user_type){
    // Inicializa USER_TYPE
    USER_TYPE = user_type;
    if(user_type == CLIENT){
        REAL_SENDER_PORT = 8074;
        REAL_RECEIVER_PORT = 8075;
    }
    else if(user_type == SERVER){
        REAL_SENDER_PORT = 8075;
        REAL_RECEIVER_PORT = 8074;
    }
    else{
        return -1;
    }

    // Inicializa vetor de SOCKETS
    for (int i = 0; i < MAX_SOCKETS; ++i)
    {
        SOCKETS[i] = NULL;
    }

    int not_error = TRUE;
    int result;
    // Inicializa as variáveis globais
    NETWORK_SEG_BUFFER = new_seg_queue();
    if(NETWORK_SEG_BUFFER == NULL) not_error = FALSE;

	NETWORK_IP_BUFFER  = new_ip_queue();
	if(NETWORK_IP_BUFFER == NULL) not_error = FALSE;

	result = pthread_mutex_init(&NETWORK_MUTEX, NULL);
	if (result != 0) not_error = FALSE;

	result = pthread_mutex_init(&DELETE_MUTEX, NULL);
	if (result != 0) not_error = FALSE;

	if(not_error == FALSE){
		seg_clear_queue(NETWORK_SEG_BUFFER);
		ip_clear_queue(NETWORK_IP_BUFFER);
		pthread_mutex_destroy(&NETWORK_MUTEX);
		return FALSE;
	}

    // Ouve da camada de rede
	result = pthread_create(&NETWORK_THREAD, NULL, listen_network_layer, NULL);
	if(result != 0) return FALSE;
    // Kernel
    result = pthread_create(&KERNEL, NULL, kernel_thread, NULL);        
    if(result != 0){
    	pthread_cancel(NETWORK_THREAD);
    	return FALSE;
    } 

    return TRUE;
}

int GBN_listen(int port){
	// Acha um id disponível
	int id = get_next_id();
	if(id == -1) return -1;

	// Cria o socket
	SOCKETS[id] = make_listener(id, port);
	if(SOCKETS[id] == NULL) return -1;

	SOCKETS[id]->state = LISTEN;

	return id;
}

int GBN_accept(int listener_id){
	if(listener_id >= MAX_SOCKETS) return -1;

	// Acha um id disponível
	int id = get_next_id();
	if(id == -1) return -1;

	GBN_Socket* listener = SOCKETS[listener_id];
	if(listener == NULL) return -1;

	if(listener->state != LISTEN) return -1;

	printf("GBN_Accept: Aguardando receber requisição...\n");

	// Aguarda enquanto a fila de requisições está vazia
	while(req_queue_isempty(listener->req_queue));

	// Tira a requisição da fila
	LOCK(listener->req_mutex);
	char remote_ip[16];
	Segment* req = req_queue_pop(listener->req_queue, remote_ip);
	UNLOCK(listener->req_mutex);

	// Cria o socket de conexão
	SOCKETS[id] = make_connection(id, req->dest_port, req->orig_port, remote_ip);
	free(req);
	if(SOCKETS[id] == NULL) return -1;

	// Muda estado para SYN_ACK
	start_timer(SOCKETS[id]->gbn_timer);
	SOCKETS[id]->state = SYN_ACK;

	printf("GBN_Accept: Aguardando estabelecer conexão...\n");

	// Aguarda ter conexão estabelecida
	while(SOCKETS[id]->state != ESTABLISHED && SOCKETS[id]->state != CLOSED);

	// Se tiver fechado, exclui o socket e retorna FALSE
	if(SOCKETS[id]->state == CLOSED){
		delete_socket(SOCKETS[id]);
		return -1;
	}
	
	// Conexão estabelecida
	printf("GBN_Connect: Conexão estabelecida.\n");
	return id;
}

int GBN_connect(int server_port, const char* server_ip){
	// Acha um id disponível
	int id = get_next_id();
	if(id == -1) return -1;

	// Acha uma porta disponível
	int local_port = get_port(server_port, server_ip);

	// Cria o socket de conexão
	SOCKETS[id] = make_connection(id, local_port, server_port, server_ip);

	if(SOCKETS[id] == NULL){
		// printf("Não criou.\n");
		return -1;
	}

	// Muda o estado para SYN_SEND
	start_timer(SOCKETS[id]->gbn_timer);
	SOCKETS[id]->state = SYN_SEND;

	printf("GBN_Connect: Aguardando estabelecer conexão...\n");

	// Aguarda ter conexão estabelecida
	while(SOCKETS[id]->state != ESTABLISHED && SOCKETS[id]->state != CLOSED);

	// Se tiver fechado, exclui o socket e retorna FALSE
	if(SOCKETS[id]->state == CLOSED){
		delete_socket(SOCKETS[id]);
		return -1;
	}
	
	// Conexão estabelecida
	printf("GBN_Connect: Conexão estabelecida.\n");
	return id;
}

int GBN_send(int socket_id, char* snd_data, int length){
	if(socket_id >= MAX_SOCKETS) return -1;
	GBN_Socket* socket = SOCKETS[socket_id];
	if(socket == NULL) return -1;

	printf("GBN_send: Enviar algo, length = %i.\n", length);

	int sent_data = 0;
	int tail_size = length % DATA_SIZE;
	int last = length - tail_size;
	printf("GBN_send: Last = %i.\n", last);
	while(sent_data < last){
		LOCK(socket->snd_mutex);
        data_queue_push(socket->snd_buffer, (snd_data + sent_data));
        UNLOCK(socket->snd_mutex);
        printf("GBN_send: Coloca dados no buffer.\n");
        sent_data += DATA_SIZE;
	}

	// Tem um resto
	if(tail_size != 0){
		printf("GBN_send: Tem rabinho.\n");
		char tail[DATA_SIZE];
        memcpy(tail, (snd_data + sent_data), tail_size);

        LOCK(socket->snd_mutex);
        data_queue_push(socket->snd_buffer, tail);
        UNLOCK(socket->snd_mutex);

        sent_data += tail_size;
	}

	return sent_data;
}

int GBN_receive(int socket_id, char* rcv_data, int length){
	if(socket_id >= MAX_SOCKETS){
		printf("GBN_receive: Socket tem id inválido.\n");
		return -1;
	}
	GBN_Socket* socket = SOCKETS[socket_id];
	if(socket == NULL){
		printf("GBN_receive: Socket com id %i passado é nulo.\n", socket_id);
		return -1;
	}

	printf("GBN_receive: Receber dados, length = %i.\n", length);
	int received_data = 0;
	int tail_size = length % DATA_SIZE;
	int last = length - tail_size;
	while(received_data < last){		
		// Aguarda ter algo no buffer
		printf("GBN_receive: Aguardando dados no buffer...\n");
		char data[DATA_SIZE];
		while(data_queue_isempty(socket->rcv_buffer));
		
		LOCK(socket->rcv_mutex);
        data_queue_pop(socket->rcv_buffer, data);
        UNLOCK(socket->rcv_mutex);

        memcpy((rcv_data + received_data), data, DATA_SIZE); 

        printf("GBN_receive: Dados coletados.\n");
        
        received_data += DATA_SIZE;
	}

	// Tem um resto
	if(tail_size != 0){		
		// Aguarda ter algo no buffer
		printf("GBN_receive: Aguardando dados no buffer...\n");
		char tail[DATA_SIZE];
		while(data_queue_isempty(socket->rcv_buffer));

		LOCK(socket->rcv_mutex);
        data_queue_pop(socket->rcv_buffer, tail);
        UNLOCK(socket->rcv_mutex);

        memcpy((rcv_data + received_data), tail, tail_size);        

        received_data += tail_size;
	}

	printf("GBN_receive: Recebeu todos os dados.\n");
	return received_data;
}

int GBN_peer_ip(int socket_id, char* ip){
    if(socket_id >= MAX_SOCKETS) return FALSE;
	GBN_Socket* socket = SOCKETS[socket_id];
	if(socket == NULL) return FALSE;

    strcpy(ip, socket->remote_IP);
    return TRUE;
}

void GBN_close(int socket_id){
	if(socket_id >= MAX_SOCKETS) return;
	GBN_Socket* socket = SOCKETS[socket_id];
	if(socket == NULL) return;
	if(socket->state != ESTABLISHED) return;

	socket->state = FIN_SEND;

	LOCK(DELETE_MUTEX);
	printf("GBN_close: Aguradando fechar a conexão...\n");
	while(socket->state != CLOSED);
	UNLOCK(DELETE_MUTEX);

	// delete_socket(SOCKETS[socket_id]);
	SOCKETS[socket_id] = NULL;
}

void GBN_transport_end(){
    pthread_cancel(NETWORK_THREAD);
	END_KERNEL = TRUE;
	pthread_join(KERNEL, NULL);
	for (int i = 0; i < MAX_SOCKETS; ++i)
	{
		if(SOCKETS[i] != NULL) delete_socket(SOCKETS[i]);
	}
}

// Ouve da camada de rede e preenchia NETWORK_BUFFER
void* listen_network_layer(void* data){
	int result;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &result);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &result);	

	while(TRUE){
		char remoteIP[16];
		Segment* seg = ALLOC(Segment);
		// printf("Network: Recebendo na porta real %u\n", REAL_RECEIVER_PORT);
        result = recv_segment((char*)seg, sizeof(Segment), remoteIP, REAL_RECEIVER_PORT);
        // printf("Network: Chegou algo do ip %s.\n", remoteIP);
        if(result == FALSE){
        	free(seg);
        	continue;
        }

        LOCK(NETWORK_MUTEX);
        seg_queue_push(NETWORK_SEG_BUFFER, seg);
        ip_queue_push(NETWORK_IP_BUFFER, remoteIP);
        UNLOCK(NETWORK_MUTEX);
	}
}

// UTILIZADO PELO KERNEL
void print_pack(Segment* seg, char* funcName); // Para DEBUG
int fin_flag(Segment* seg);
int syn_flag(Segment* seg);
int ack_flag(Segment* seg);
int is_corrupted(Segment* seg);
Segment* make_seg(	uint32_t seq_number, 
					uint16_t orig_port, 
					uint16_t dest_port, 
					uint16_t flags, 
					uint8_t* data);

// KERNEL
void * kernel_thread(void* data){
	while(TRUE){
		if(END_KERNEL) break;		

		// If pra saber se chegou algo
		Segment* seg;
		char IP[16];
		int has_segment = FALSE;
		if(!seg_queue_isempty(NETWORK_SEG_BUFFER)){			
			LOCK(NETWORK_MUTEX);
			seg = seg_queue_pop(NETWORK_SEG_BUFFER);
			ip_queue_pop(NETWORK_IP_BUFFER, IP);
			UNLOCK(NETWORK_MUTEX);

			if(is_corrupted(seg)){
				free(seg);
				has_segment = FALSE;
			}
			else{
				has_segment = TRUE;		
				// printf("KERNEL: Pacote recebido:\n");
				// print_pack(seg, "KERNEL");
			}
		}

		// For nos sockets pra saber o que fazer,
		// dependendo do estado e para verificar os timeouts
		for (int i = 0; i < MAX_SOCKETS; ++i)
		{
			if(SOCKETS[i] != NULL){
				GBN_Socket* s = SOCKETS[i];
				if(s->state == CLOSED){
					LOCK(DELETE_MUTEX);
					printf("Closed.\n");
					delete_socket(s);
					UNLOCK(DELETE_MUTEX);
					continue;
				}
				switch(s->state){
					case LISTEN:
						if(has_segment && syn_flag(seg) && seg->dest_port == s->local_port){
							LOCK(s->req_mutex);
							req_queue_push(s->req_queue, seg, IP);
							UNLOCK(s->req_mutex);
						}
					break;

					case ESTABLISHED:
					case SYN_SEND:					
					case SYN_SENT:
					case SYN_ACK:
					case SYN_RCVD:
					case CLOSE_WAIT:
					case FIN_WAIT_1:
					case FIN_WAIT_2:
					case LAST_ACK:
					case TIME_WAIT:
					case FIN_SEND:
						// REMETENTE
						// Se tem que enviar algo
						// if(s->state == ESTABLISHED) printf("Kernel: ESTABLISHED\n");
						// if(!data_queue_isempty(s->snd_buffer)) printf("Kernel: snd_buffer não ta vazio.\n");
						if(s->state == TIME_WAIT){
							if(timeout(s->close_timer)){
								s->state = CLOSED;
							}
						}
						else if(s->nextseqnum < s->base+WINDOW_SIZE){
							if( !data_queue_isempty(s->snd_buffer) 
								|| s->state == SYN_SEND
								|| s->state == SYN_SENT
								|| s->state == FIN_SEND
								|| s->state == FIN_WAIT_1
								|| s->state == FIN_WAIT_2
								|| s->state == TIME_WAIT
								|| s->state == CLOSE_WAIT
							  )
							{
								uint8_t 	data[DATA_SIZE];
								uint8_t  	flags_sent;
								uint8_t* 	data_sent;
								int send = TRUE;
								if(s->state == SYN_SEND){
									flags_sent = SYN_FLAG;
									data_sent = NULL;
									s->state = SYN_SENT;
									// printf("Kernel: Estado SYN_SEND.\n");
								}								
		                		else if(s->state == SYN_SENT){	                			
		                			if(has_segment && ack_flag(seg) && syn_flag(seg)){
		                				flags_sent = ACK_FLAG;
		                				data_sent = NULL;		                			
		                				s->state = ESTABLISHED;
		                			}else{
		                				send = FALSE;
		                			}	                			
		                		}		             
								else if(s->state == ESTABLISHED){																
									LOCK(s->snd_mutex);
									data_queue_pop(s->snd_buffer, data);
									UNLOCK(s->snd_mutex);

									flags_sent = 0;
									data_sent = data;
								}
								else if(s->state == FIN_SEND){
									printf("FIN_SEND\n");
									flags_sent = FIN_FLAG;
									data_sent = NULL;
									s->state = FIN_WAIT_1;
								}
								else if(s->state == FIN_WAIT_1){
									printf("FIN_WAIT_1\n");
									if(has_segment && ack_flag(seg)){
										s->state = FIN_WAIT_2;
									}
									else send = FALSE;
								}
								else if(s->state == FIN_WAIT_2){
									printf("FIN_WAIT_2\n");
									if(has_segment && fin_flag(seg)){
										printf("Vai pra TIME_WAIT\n");
										flags_sent = ACK_FLAG;
		                				data_sent = NULL;
		                				send = TRUE;
		                				s->state = TIME_WAIT;
		                				start_timer(s->close_timer);
									}
									else send = FALSE;
								}
								else if(s->state == CLOSE_WAIT){
		                			printf("CLOSE_WAIT\n");
		                			flags_sent = FIN_FLAG;
		                			data_sent = NULL;			                	
			                		s->state = LAST_ACK;
		                		}

								
								if(send){
									// Cria pacote com numero de sequencia nextseqnum
				                    // e com os dados retirados da pilha
				                    s->window[s->nextseqnum%WINDOW_SIZE] = make_seg(s->nextseqnum, s->local_port, s->remote_port, flags_sent, data_sent);
				                    // printf("Kernel: Enviou pacote:\n");
				                    // print_pack(s->window[s->nextseqnum%WINDOW_SIZE], "Kernel");
				                    // Envia pacote para a camada de rede
				                    // printf("Kernel: Mandando para %s\n", s->remote_IP);
				                    // printf("Kernel: Porta destino real %u\n", REAL_SENDER_PORT);
				                    send_segment((char*)s->window[s->nextseqnum%WINDOW_SIZE], sizeof(Segment), s->remote_IP, REAL_SENDER_PORT);			                 
				                    
				                    if(s->base == s->nextseqnum){
				                        start_timer(s->gbn_timer);
				                    }

				                    s->nextseqnum++;
								}			                    
							}	                    
	                	}
	                	
	                	// Se recebeu um ack	                	
	                	if(has_segment && ack_flag(seg)){	                		
	                		s->base = seg->seq_number + 1;
	                		if(s->base == s->nextseqnum){
	                			stop_timer(s->gbn_timer);
	                		}
	                		else{
	                			start_timer(s->gbn_timer);
	                		}
	                	}

	                	// Se deu timeeout
	                	if(timeout(s->gbn_timer)){
	                		start_timer(s->gbn_timer);
			                // printf("SENDER_THREAD: Reinicia o timer\n");
			                // printf("SENDER_THREAD: Reenvia janela:\n");
			                for(int i = s->base; i < s->nextseqnum; i++){
			                    // printf("Kernel: Pacote reenviado %u:\n", i);
			                    // print_pack(s->window[i%WINDOW_SIZE], "Kernel");
			                    send_segment((char*)s->window[i%WINDOW_SIZE], sizeof(Segment), s->remote_IP, REAL_SENDER_PORT);          
			                    // printf("SENDER_THREAD: Enviado.\n");
			                }
	                	}

	                	// DESTINATÁRIO
	                	// Se chegou dados

	                	if(s->state == SYN_ACK){
                			uint8_t flags = ACK_FLAG | SYN_FLAG;
                			s->state = SYN_RCVD;
                			free(s->ack);
	                		s->ack = make_seg(s->expectedseqnum, s->local_port, s->remote_port, flags, NULL);	                			
	                		// printf("Receiver: Enviou pacote:\n");
	                		// print_pack(s->ack, "Receiver");
	                		send_segment((char*)s->ack, sizeof(Segment), s->remote_IP, REAL_SENDER_PORT);

                			s->expectedseqnum++;
                		}

                		
                		// if(has_segment == TRUE) printf("Receiver: Tem dados pra receber.\n");            		
                		// if(syn_flag(seg))  
                			// printf("Receiver: É SYN.\n");
	                	if(has_segment && !syn_flag(seg) && s->state != CLOSE_WAIT){                		
	                		// Se o numero é esperado
		                	if(seg->seq_number == s->expectedseqnum){		                		
		                		int send_ack = TRUE;
		                		if(s->state == SYN_RCVD && ack_flag(seg)){
		                			// printf("Receiver: Chegou!\n");
		                			send_ack = FALSE;
		                			s->state = ESTABLISHED;		                			
		                		}
		                		else if(s->state == ESTABLISHED && seg->flags == 0){
		                			// printf("Receiver: Estado é ESTABLISHED.\n");
		                			LOCK(s->rcv_mutex);
	                				data_queue_push(s->rcv_buffer, seg->data);
	                				UNLOCK(s->rcv_mutex);

	                				// printf("Receiver: Deu push no rcv_buffer.\n");	                				
		                		}
		                		else if(s->state == ESTABLISHED && fin_flag(seg)){
		                			printf("Recebeu um fin_flag\n");
		                			s->state = CLOSE_WAIT;		                			
		                		}
		                		else if(s->state == LAST_ACK && ack_flag(seg)){
		                			printf("LAST_ACK\n");
		                			s->state = CLOSED;
		                			send_ack = FALSE;
		                		}
	                			
	                			if(send_ack){
	                				free(s->ack);
	                				s->ack = make_seg(s->expectedseqnum, s->local_port, s->remote_port, ACK_FLAG, NULL);	                			
	                				// printf("Receiver: Enviou pacote:\n");
	                				// print_pack(s->ack, "Receiver");
	                				send_segment((char*)s->ack, sizeof(Segment), s->remote_IP, REAL_SENDER_PORT);
	                			}	                			


	                			s->expectedseqnum++;
	                			// printf("Receiver: Incrementa expected = %u\n", s->expectedseqnum);
	                		}
	                		// Se não for o ack esperado
	                		else if (seg->flags == 0){
	                			// printf("Não é numero esperado.\n");
	                			// printf("SeqNumber = %u != %u = Expected\n", seg->seq_number, s->expectedseqnum);
	                			// printf("Receiver: Enviou pacote:\n");
	                			// print_pack(s->ack, "Receiver");                			
	                			send_segment((char*)s->ack, sizeof(Segment), s->remote_IP, REAL_SENDER_PORT);
	                		}
	                	}
	                break;				
				}
			}
		}

		if(has_segment){
			free(seg);
			seg = NULL;
		}
	}
}


// DEFINIÇÃO DAS FUNÇÕES UTILIZADAS

int get_next_id(){
	int id = -1;
	for (int i = 0; i < MAX_SOCKETS; ++i)
	{
		if(SOCKETS[i] == NULL){
			id = i;
			break;
		}
	}
	return id;
}

int get_port(int remote_port, const char* remote_ip){
	// Procura uma porta que nao entre em conflito
    // com as conexões que já existem
    uint16_t port;
    while(TRUE){
        int found = 1;
        srand((unsigned)time(NULL));
        port = 1 + (rand() % MAX_PORT);
        for (int i = 0; i < MAX_SOCKETS; ++i)
        {   
            if( SOCKETS[i] != NULL &&
                SOCKETS[i]->local_port == port &&
                SOCKETS[i]->remote_port == remote_port &&
                strcmp(SOCKETS[i]->remote_IP, remote_ip) == 0)
            {
                found = 0;
                break;
            }
        }
        if(found) break;
    }
    return port;
}

void delete_socket(GBN_Socket* socket){
	if(socket)
	{
		printf("DELETE SOCKET\n");
		delete_timer(socket->TTL);
    	delete_timer(socket->close_timer);
    	delete_timer(socket->gbn_timer);
    	req_clear_queue(socket->req_queue);
    	data_clear_queue(socket->snd_buffer);
    	data_clear_queue(socket->rcv_buffer);
    	pthread_mutex_destroy(&socket->req_mutex);
    	pthread_mutex_destroy(&socket->snd_mutex);
    	pthread_mutex_destroy(&socket->rcv_mutex);
    	free(socket->ack);
    	for (int i = 0; i < WINDOW_SIZE; ++i)
    		free(socket->window[i]);
    	socket = NULL;
	}
}

GBN_Socket* make_listener(int id, uint16_t local_port){
	int not_error = TRUE;
    GBN_Socket* socket = ALLOC(GBN_Socket);

    if(socket != NULL){

    	socket->id = id;
    	socket->local_port = local_port;
    	socket->remote_port = 0;
    	memset(socket->remote_IP, 0, 16);
    	socket->state = UNSET; 

    	socket->req_queue = new_req_queue();	
    	if(socket->req_queue == NULL) not_error = FALSE;

    	if (pthread_mutex_init(&socket->req_mutex, NULL) != 0) not_error = FALSE;

    } else not_error = FALSE;

    if(not_error == FALSE){
    	delete_socket(socket);
    	socket = NULL;
    }

    return socket;
}

GBN_Socket* make_connection(int id, uint16_t local_port, uint16_t remote_port, const char* remote_IP){
	int not_error = TRUE;
    GBN_Socket* socket = ALLOC(GBN_Socket);

    if(socket != NULL){
    	// printf("Alocou socket.\n");
    	socket->id = id;
    	socket->local_port = local_port;
    	socket->remote_port = remote_port;
    	strcpy(socket->remote_IP, remote_IP);
    	socket->state = UNSET;

    	socket->TTL = new_timer(240000);
    	if(socket->TTL == NULL) not_error = FALSE;
    	// if(not_error) printf("Criou TTL\n");
    	
    	socket->close_timer = new_timer(30000);
    	if(socket->close_timer == NULL) not_error = FALSE;
    	// if(not_error) printf("Criou close_timer_timer\n");

    	socket->snd_buffer = new_data_queue();
    	if(socket->snd_buffer == NULL) not_error = FALSE;
    	// if(not_error) printf("Criou snd_buffer\n");

    	socket->rcv_buffer = new_data_queue();
    	if(socket->rcv_buffer == NULL) not_error = FALSE;
    	// if(not_error) printf("Criou rcv_buffer\n");

    	if (pthread_mutex_init(&socket->snd_mutex, NULL) != 0) not_error = FALSE;
    	// if(not_error) printf("Criou snd_mutex\n");

    	if (pthread_mutex_init(&socket->rcv_mutex, NULL) != 0) not_error = FALSE;
    	// if(not_error) printf("Criou rcv_mutex\n");

    	socket->nextseqnum = 1;
    	socket->base = 1;
    	memset(socket->window, 0, WINDOW_SIZE);

    	socket->gbn_timer = new_timer(1000);
    	if(socket->gbn_timer == NULL) not_error = FALSE;
    	// if(not_error) printf("Criou gbn_timer\n");

    	socket->expectedseqnum = 1;
    	socket->ack = make_seg(0,socket->local_port, socket->remote_port, ACK_FLAG, NULL);

    } else not_error = FALSE;

    if(not_error == FALSE){
    	delete_socket(socket);
    	socket = NULL;
    }

    return socket;
}

void print_pack(Segment* seg, char* funcName){
    printf("============\n");
    printf("%s: |SeqNumber = %u |\n", funcName, seg->seq_number);
    printf("%s: |OrigPort = %u |\n", funcName, seg->orig_port);
    printf("%s: |DestPort = %u |\n", funcName, seg->dest_port);
    printf("%s: |Flags = 0x%x |\n", funcName, seg->flags);
    printf("%s: |Data = ", funcName);
    for (int i = 0; i < DATA_SIZE; ++i)
    {
        if(33 <= seg->data[i] &&  seg->data[i] <= 126 && seg->data[i] != ' ')
            printf("%c ", seg->data[i]);
        else if(seg->data[i] == ' ')
            printf("[SPC] ");
        else
            printf("0x%x ", seg->data[i]);
    }
    printf("|\n");
    printf("============\n");
}

int fin_flag(Segment* seg){ 
	if(seg != NULL)
		return (seg->flags & FIN_FLAG);
	else return FALSE;
}

int syn_flag(Segment* seg){ 
	if(seg != NULL)
		return (seg->flags & SYN_FLAG);
	else return FALSE;
}

int ack_flag(Segment* seg){ 
	if(seg != NULL)
		return (seg->flags & ACK_FLAG);
	else return FALSE;
}

int is_corrupted(Segment* seg){
    uint32_t aux;
    uint16_t sum = 0;

    uint8_t* data;
    data = (uint8_t*) seg;

    for (int i = 0; i < SEG_SIZE ; i+=2)
    {
        // printf("data[%i] = %x\n", i, data[i]);
        // printf("data[%i] = %x\n", i+1, data[i+1]);
        uint16_t operand = data[i+1];
        operand = operand << 8;
        operand |= data[i];
        // printf("operando = %x\n", operand);
        aux = (uint32_t)sum + (uint32_t)operand;
        if(aux >> 16 == 1){
            aux += 1;
        }
        sum = aux;
    }

    if(sum == 0xffff) return FALSE;
    else return TRUE;
}

void make_checksum(Segment* seg){
    seg->checksum = 0;
    
    uint32_t aux;
    uint16_t sum = 0;    
    uint8_t* data;
    data = (uint8_t*) seg;
    
    for(int i = 0; i < SEG_SIZE; i+=2){
        uint16_t operand = data[i+1];
        operand = operand << 8;
        operand |= data[i];
        aux = (uint32_t)sum + (uint32_t)operand;
        if(aux >> 16 ==1){
            aux += 1;
        }
        sum = aux;
    }
    
    seg->checksum = ~sum;
}

Segment* make_seg(uint32_t seq_number, uint16_t orig_port, uint16_t dest_port, uint16_t flags, uint8_t* data){
    Segment* seg = ALLOC(Segment);
    if(seg == NULL) return NULL;

    seg->seq_number = seq_number;
    seg->orig_port = orig_port;
    seg->dest_port = dest_port;
    seg->flags = flags;
    if(data != NULL) memcpy((void*)seg->data, (void*)data, DATA_SIZE);

    make_checksum(seg);

    return seg;
}


