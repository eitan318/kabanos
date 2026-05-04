#include "proc/sys_wait.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "syscall_errno.h"
#include <klib/stdio.h>

pid_t sys_waitpid(pid_t target_pid, int *wstatus, int options) {
  thread_t *current = dispatch_get_current();
  process_t *caller = current->process;

  while (1) {
    bool has_matching_child = false;
    process_t *prev = NULL;
    process_t *p = caller->first_child;

    while (p) {

      // Target -1 means on any child,
      if (target_pid == -1 || p->pid == target_pid) {
        has_matching_child = true;
        if (p->state == PROCESS_ZOMBIE) {
          pid_t pid = p->pid;
          int exit_status = p->exit_code;

          // 2. Unlink from list
          if (prev)
            prev->next_sibling = p->next_sibling;
          else
            caller->first_child = p->next_sibling;

          // 3. Copy status to userspace (requires safety checks!)
          if (wstatus != NULL) {
            *wstatus = exit_status;
          }

          // 4. Clean up memory
          process_destroy(p);
          return pid;
        }
      }
      prev = p;
      p = p->next_sibling;
    }

    if (!has_matching_child) {
      return -ECHILD;
    }

    // If WNOHANG is set, return 0 (child exists but alive)
    if (options & WNOHANG) {
      return 0;
    }

    // Block and wait for a signal from sys_exit
    caller->is_waiting = 1;
    sched_dequeue(current);
    sched_switch_next();
    caller->is_waiting = 0;

    // After waking up, the loop runs again to find the now-zombie child(after
    // the child exited)
  }
}
