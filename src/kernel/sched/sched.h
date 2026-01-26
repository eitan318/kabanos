#include "proc/proc.h"
#include "sched/thread.h"

thread_t *sched_next(void);
void sched_add(thread_t *t);
void sched_init();
