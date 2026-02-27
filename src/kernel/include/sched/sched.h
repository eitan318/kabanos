#include "sched/thread.h"

#define TIMER_TICK_MS 1

void sched_yield();
void sched_enqueue(thread_t *t);
void sched_dequeue(thread_t *t);
thread_t *sched_pick_next(void);
void sched_tick(void *context);
uint32_t sched_time_get();
void sys_yield();
