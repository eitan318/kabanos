#include "hal.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"

void handle_yield(void *context) {
  thread_t *current = dispatch_get_current();

  if (current->tid != 0) { // don't enqueue the idle task
    sched_enqueue(current);
  }
  thread_t *next = sched_pick_next();
  thread_t *curr = dispatch_get_current();
  hal_thread_save(curr->arch, context);
  dispatch_switch_to(next);
}
