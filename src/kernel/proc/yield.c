#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"

void handle_yield(void) {
  thread_t *current = dispatch_get_current();

  if (current->tid != 0) { // Don't enqueue the idle task
    sched_enqueue(current);
  }
  thread_t *next = sched_pick_next();
  dispatch_switch_to(next);
}
