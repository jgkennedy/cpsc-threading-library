#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> 
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "mythreads.h"

#define BIG 1000
#define TIMER_INTERVAL_US 10

ucontext_t main_context, thread_context;

void timer_handler (int signum) 
{
	if (!interruptsAreDisabled)
	{
		printf(" interrupting\n");
		threadYield();
	}
} 

//setup the timer 
void set_timer()
{
	struct sigaction sa; 
	struct itimerval timer; 

	/* Install timer_handler as the signal handler for SIGVTALRM.  */ 
	memset (&sa, 0, sizeof (sa)); 
	sa.sa_handler = &timer_handler; 
	sigaction (SIGALRM, &sa, NULL); 

  	/* Configure the timer to expire after TIMER_INTERVAL_US usec  */ 
	timer.it_value.tv_sec = 0; 
	timer.it_value.tv_usec = TIMER_INTERVAL_US; 

	/* and every TIMER_INTERVAL_US usec after that.  */ 
	timer.it_interval.tv_sec = 0; 
	timer.it_interval.tv_usec = TIMER_INTERVAL_US; 

  	/* Start a virtual timer. It counts down whenever this process is executing.  */ 
	if (0 != setitimer (ITIMER_REAL, &timer, NULL))
	{
		perror("setitimer error");
	} 
}

int main(void)
{
	int id1, id2;
	int p1;
	int p2;

	p1 = 23;
	p2 = 2;

	int *result1, *result2;

	//initialize the threading library. DON'T call this more than once!
	threadInit();

	//always start the timer after threadInit
	set_timer();

	id1 = threadCreate(/*some func*/,(void*)&p1);
	printf("created thread 1.\n");	
	
	id2 = threadCreate(/*some func*/,(void*)&p2);
	printf("created thread 2.\n");

	threadJoin(id1, (void*)&result1);
	printf("joined #1 --> %d.\n",*result1);

}

extern void threadInit() {

 NODE *front,*rear,*temp,*front1;

}

extern int threadCreate(thFuncPtr funcPtr, void *argPtr) {

 char * myStack = malloc(STACK_SIZE);
 
 getcontext(&thread_context);
		
 thread_context.uc_stack.ss_sp = myStack;
 thread_context.uc_stack.ss_size = STACK_SIZE;

 thread_context.uc_link = &main_context;
 
 makecontext(&thread_context, (void (*)(void) funcPtr, 1, *argPtr);
 
 
 
 

}
