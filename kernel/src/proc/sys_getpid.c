/**
 * @file sys_getpid.c
 * @brief getpid syscall.
 */
#include "proc/proc.h"
#include "sched/dispatcher.h"

long sys_getpid() {
  thread_t *current = dispatch_get_current();
  return current->process->pid;
}
