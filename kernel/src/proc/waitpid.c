#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "syscall_errno.h"

#ifndef WNOHANG
#define WNOHANG 1
#endif

pid_t sys_waitpid(pid_t target_pid, int *wstatus, int options) {
  thread_t *current = dispatch_get_current();
  process_t *caller = current->process;

  while (1) {
    bool has_matching_child = false;
    process_t *zombie = NULL;
    process_t *prev = NULL;
    process_t *p = caller->first_child;

    while (p) {
      if (target_pid == -1 || p->pid == target_pid) {
        has_matching_child = true; // We found the child!
        if (p->state == PROCESS_ZOMBIE) {
          zombie = p;
          break;
        }
      }
      prev = p;
      p = p->next_sibling;
    }

    if (zombie) {
      pid_t zpid = zombie->pid;

      // 2. Safe pointer access (Simplified here, but use a check in real code)
      if (wstatus)
        *wstatus = (zombie->exit_code & 0xff) << 8;

      // 3. REMOVE from linked list before destroying
      if (prev)
        prev->next_sibling = zombie->next_sibling;
      else
        caller->first_child = zombie->next_sibling;

      process_destroy(zombie);
      return zpid;
    }

    // If we got here, no zombie was found.
    // CHECK 1: If the child doesn't even exist, return error
    if (!has_matching_child) {
      return -ECHILD;
    }

    // CHECK 2: If WNOHANG is set, return 0 (child exists but alive)
    if (options & WNOHANG) {
      return 0;
    }

    // CHECK 3: Block and wait for a signal from sys_exit
    caller->is_waiting = 1;
    sched_dequeue(current);
    sched_yield();
    caller->is_waiting = 0;

    // After waking up, the loop runs again to find the now-zombie child
  }
}
