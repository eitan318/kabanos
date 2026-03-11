#include "panic.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"

void sys_exit(int status) {
  thread_t *current = dispatch_get_current();
  process_t *proc = current->process;

  proc->exit_code = status;
  proc->state = PROCESS_ZOMBIE;

  // Tell the scheduler never to run this thread again
  sched_dequeue(current);

  // If the parent is waiting, wake them up
  if (proc->parent && proc->parent->is_waiting) {
    sched_enqueue(proc->parent->main_thread);
  }

  sched_switch_next();
  panic("returned from exit!");
}
