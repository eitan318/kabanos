#include "proc/proc.h"
#include "sched/thread.h"

#define QUANTOM_TIME_NORMAL 50
#define QUANTOM_TIME_ABOVE_NORMAL 150
#define QUANTOM_TIME_HIGH 200
#define QUANTOM_TIME_REALTIME 400

void sched_add(thread_t *t);
void sched_remove(thread_t *t);
void sched_tick(void *context);
void sched_init();
