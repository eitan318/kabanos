/**
 * @file sys_fork.c
 * @brief fork syscall: clones the process, its address space (COW) and
 *        its main thread.
 */
#include "fs/vfs.h"
#include "hal.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"

long sys_fork() {
  process_t *parent_proc = dispatch_get_current()->process;

  process_t *child_proc = process_create();
  child_proc->parent = parent_proc;

  // Enqueue child to parent list
  child_proc->next_sibling = parent_proc->first_child;
  parent_proc->first_child = child_proc;

  child_proc->cwd = parent_proc->cwd;
  vfs_vnode_get_ref(child_proc->cwd);

  child_proc->vmspace = vmspace_clone(parent_proc->vmspace);

  thread_t *child_thread = thread_clone(dispatch_get_current(), child_proc);
  child_proc->main_thread = child_thread;

  hal_thread_set_return_value(child_thread, 0);

  sched_enqueue(child_thread);

  return child_proc->pid; // Parent gets the PID
}
