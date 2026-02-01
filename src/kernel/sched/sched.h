#include "proc/proc.h"
#include "sched/thread.h"

void sched_add(thread_t *t);
void sched_tick(void *context);
void sched_init();
