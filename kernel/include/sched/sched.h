#include "sched/thread.h"

void sched_switch_next();
void sched_enqueue(thread_t *t);
void sched_dequeue(thread_t *t);
thread_t *sched_pick_next(void);
void sched_on_timer_tick(void *context);
