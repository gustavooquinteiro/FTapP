#include "../include/timer.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

struct timer
{
	pthread_t 	thread;
	unsigned	time_limit;

	char		timeout;
	char		stop;
	char		start;
	char		close;
};


void* timer_thread(void* data){
	Timer* t = (Timer*) data;

	int run_timer = 0;
    int time;
    int endpoint = t->time_limit;
    clock_t initial;
    clock_t difference;

	while(1){
		if(t->close) break;

		if(t->stop){
        	t->stop = 0;
        	t->timeout = 0;
        	time = 0;
        	run_timer = 0;
        }
        if(t->start){
        	t->start = 0;
        	t->timeout = 0;
        	initial = clock();
        	run_timer = 1;
        }

		if(run_timer){
            difference = clock() - initial;
            time = difference * 1000 / CLOCKS_PER_SEC; 
        }
        if(time >= endpoint){
        	t->timeout = 1;
        }
	}
}

Timer* new_timer(unsigned time_limit){
	Timer* new = (Timer*) malloc(sizeof(Timer));
	if(new == NULL) return NULL;
	if(time_limit == 0) return NULL;
	
	new->time_limit = time_limit;
	new->timeout = 0;
	new->stop = 0;
	new->start = 0;
	new->close = 0;

	int result = pthread_create(&new->thread, NULL, timer_thread, (void*) new);
	if(result != 0) return NULL;

	return new;
}

int start_timer(Timer* timer){
	if(timer == NULL) return 0;
	timer->start = 1;
	return 1;
}

int  stop_timer(Timer* timer){
	if(timer == NULL) return 0;
	timer->stop = 1;
	return 1;
}

void  delete_timer(Timer* timer){
	if(timer){
		// Cancela a thread
		timer->close = 1;
		pthread_join(timer->thread, NULL);
		free(timer);
	}	
}

int timeout(Timer* timer){
	if(timer == NULL) return -1;
	
	char timeout = timer->timeout;
	if(timeout){
		stop_timer(timer);
	}
	return timeout;
}
