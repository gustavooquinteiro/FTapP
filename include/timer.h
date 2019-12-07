#ifndef _TIMER_H
#define _TIMER_H

typedef struct timer Timer;

Timer* new_timer(unsigned time_limit);
int start_timer(Timer*);
int stop_timer(Timer*);
void delete_timer(Timer*);

int timeout(Timer*);

#endif