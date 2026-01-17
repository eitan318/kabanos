#include "proc/proc.h"
#include "sched/thread.h"

#define PREEMPTIVE_INT 45

thread_t *sched_next(void);
void sched_add(thread_t *t);
void yield(void);
void sched_init();
