#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"

void sys_yield() {
  thread_t *curr = dispatch_get_current();
  sched_enqueue(curr);
  sched_switch_next();
}
