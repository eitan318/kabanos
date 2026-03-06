#include "proc/waitpid.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"

#ifndef WNOHANG
#define WNOHANG 1
#endif

pid_t sys_waitpid(pid_t target_pid, int *wstatus, int options) {
  thread_t *current = dispatch_get_current();
  process_t *caller = current->process;

  while (1) {
    process_t *zombie = NULL;
    process_t *p = caller->first_child;
    while (p) {
      if (p->state == PROCESS_ZOMBIE) {
        if (target_pid == -1 || p->pid == target_pid) {
          zombie = p;
          break;
        }
      }
      p = p->next_sibling;
    }

    if (zombie) {
      pid_t zpid = zombie->pid;
      if (wstatus)
        *wstatus = (zombie->exit_code & 0xff) << 8;
      process_destroy(zombie);
      return zpid;
    }

    if (options & WNOHANG)
      return 0;

    caller->is_waiting = 1;
    sched_dequeue(current);
    sched_yield();
    caller->is_waiting = 0;
  }
}
