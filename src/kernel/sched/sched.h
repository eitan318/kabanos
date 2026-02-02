#include "proc/proc.h"
#include "sched/thread.h"

struct regs;

void sched_add(thread_t *t);
void sched_remove(thread_t *t);
void sched_tick(struct regs *r);
void sched_init();
