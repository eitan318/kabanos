#include "hal.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"

void sys_yield(void *context) {
  thread_t *next = sched_pick_next();
  dispatch_switch_from_interrupt(context, next);
}
