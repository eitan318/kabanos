#include "proc/proc.h"
#include "sched/thread.h"

#define QUANTOM_TIME_NORMAL 25
#define QUANTOM_TIME_ABOVE_NORMAL 50
#define QUANTOM_TIME_HIGH 125
#define QUANTOM_TIME_REALTIME 100

// Aging: how many ticks a READY thread must wait before it gets bumped up one level 
#define AGING_THRESHOLD 200

// Demotion: how many ticks a thread can stay at a priority before it drops one level 
#define DEMOTION_THRESHOLD 200

// Aging Limit is the priority that threads cannot age above it 
#define AGING_LIMIT THREAD_HIGH

// Original priority — demotion floor
#define BASE_THREAD_PRIORITY THREAD_NORMAL

void sched_add(thread_t *t);
void sched_remove(thread_t *t);
void sched_tick(struct regs *r);
void sched_init();
