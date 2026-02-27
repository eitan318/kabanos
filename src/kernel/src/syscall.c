#include "syscall.h"
#include "device.h"
#include "fs/open.h"
#include "fs/read_write.h"
#include "hal.h"
#include "klib/stddef.h"
#include "klib/stdint.h"
#include "klib/stdio.h"
#include "klib/stdlib.h"
#include "klib/string.h"
#include "proc/exec.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/sleep.h"
#include "sched/thread.h"

typedef enum {
  /* --- File & Device I/O --- */
  SYSCALL_NUMBER_SYS_WRITE = 1,
  SYSCALL_NUMBER_SYS_READ = 4,
  SYSCALL_NUMBER_SYS_OPEN = 5,
  SYSCALL_NUMBER_SYS_CLOSE = 6,
  SYSCALL_NUMBER_SYS_LSEEK = 7,
  SYSCALL_NUMBER_SYS_STAT = 8,

  /* --- Process Management --- */
  SYSCALL_NUMBER_SYS_FORK = 10,
  SYSCALL_NUMBER_SYS_EXECVE = 11,
  SYSCALL_NUMBER_SYS_EXIT = 12,
  SYSCALL_NUMBER_SYS_WAITPID = 13,
  SYSCALL_NUMBER_SYS_GETPID = 14,

  /* --- Scheduling & Time --- */
  SYSCALL_NUMBER_SYS_YIELD = 2,
  SYSCALL_NUMBER_SYS_SLEEP = 3,
  SYSCALL_NUMBER_SYS_NANOSLEEP = 15,
  SYSCALL_NUMBER_SYS_GETTIMEOFDAY = 16,

  /* --- Memory Management --- */
  SYSCALL_NUMBER_SYS_SBRK = 20, // For malloc/heap expansion
  SYSCALL_NUMBER_SYS_MMAP = 21,
  SYSCALL_NUMBER_SYS_MUNMAP = 22,

  /* --- Signals & IPC --- */
  SYSCALL_NUMBER_SYS_KILL = 30,
  SYSCALL_NUMBER_SYS_PIPE = 31,
  SYSCALL_NUMBER_SYS_SIGACTION = 32,
} SYSCALL_NUMBER;

void sys_yield() {
  thread_t *curr = dispatch_get_current();
  sched_enqueue(curr);
  sched_yield();
}

void sys_sleep(uint32_t seconds) {
  thread_t *current = dispatch_get_current();
  uint32_t curr_tick = sched_time_get();
  enqueue_sleeper(current, curr_tick + ((seconds * 1000) / TIMER_TICK_MS));
  sched_yield();
}

long sys_waitpid(int pid, int *wstatus, int options) {
  // TODO: Implement waitpid syscall
  return -1;
}

void *sys_sbrk(intptr_t increment) {
  // TODO: Implement sbrk syscall
  return (void *)-1;
}

long syscall_dispatch(syscall_info_t f) {
  switch (f.num) {
  case SYSCALL_NUMBER_SYS_WRITE:
    return sys_write(f.args[0], (const char *)f.args[1], f.args[2]);
  case SYSCALL_NUMBER_SYS_READ:
    return sys_read(f.args[0], (char *)f.args[1], f.args[2]);
  case SYSCALL_NUMBER_SYS_YIELD:
    sys_yield();
    return 0;
  case SYSCALL_NUMBER_SYS_SLEEP:
    sys_sleep(f.args[0]);
    return 0; // Should never reach here
  case SYSCALL_NUMBER_SYS_FORK:
    return sys_fork();
  case SYSCALL_NUMBER_SYS_EXECVE:
    return sys_execve((const char *)f.args[0], (char *const *)f.args[1],
                      (char *const *)f.args[2]);
  case SYSCALL_NUMBER_SYS_EXIT:
    sys_exit(f.args[0]);
    return 0; // Should never reach here
  case SYSCALL_NUMBER_SYS_WAITPID:
    return sys_waitpid(f.args[0], (int *)f.args[1], f.args[2]);
  case SYSCALL_NUMBER_SYS_SBRK:
    return (long)sys_sbrk(f.args[0]);
  case SYSCALL_NUMBER_SYS_OPEN:
    return sys_open((const char *)f.args[0], f.args[1]);
  case SYSCALL_NUMBER_SYS_CLOSE:
    return sys_close(f.args[0]);
  default:
    return -1;
  }
}
