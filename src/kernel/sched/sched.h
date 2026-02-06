#include "sched/thread.h"
#include "spinlock.h"

// initializes the ready queue and idle task
void sched_init(void);

// adds a thread to the back of the fcfs queue (admissions)
void sched_enqueue(thread_t *t);

// removes a thread from the queue (e.g., if it exits or blocks)
void sched_dequeue(thread_t *t);

// the core logic: returns the next thread that should run
// this is the "decision maker"
thread_t *sched_pick_next(void);

void sched_tick(void *context);
