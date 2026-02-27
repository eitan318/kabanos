#include "syscall.h"
#include "device.h"
#include "fs/fs_syscalls.h"
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
  SYSCALL_NUMBER_SYS_ITER_DIR = 9,
  SYSCALL_NUMBER_SYS_CREATE = 17,
  SYSCALL_NUMBER_SYS_UNLINK = 18,
  SYSCALL_NUMBER_SYS_RENAME = 19,
  SYSCALL_NUMBER_SYS_MKDIR = 23,
  SYSCALL_NUMBER_SYS_RMDIR = 24,
  SYSCALL_NUMBER_SYS_SYMLINK = 25,
  SYSCALL_NUMBER_SYS_READLINK = 26,
  SYSCALL_NUMBER_SYS_MOUNT = 27,
  SYSCALL_NUMBER_SYS_UMOUNT = 28,

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
  SYSCALL_NUMBER_SYS_SBRK = 20,
  SYSCALL_NUMBER_SYS_MMAP = 21,
  SYSCALL_NUMBER_SYS_MUNMAP = 22,

  /* --- Signals & IPC --- */
  SYSCALL_NUMBER_SYS_KILL = 30,
  SYSCALL_NUMBER_SYS_PIPE = 31,
  SYSCALL_NUMBER_SYS_SIGACTION = 32,
} SYSCALL_NUMBER;

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
    return -1;
  case SYSCALL_NUMBER_SYS_SBRK:
    return -1;
  case SYSCALL_NUMBER_SYS_OPEN:
    return sys_open((const char *)f.args[0], f.args[1]);
  case SYSCALL_NUMBER_SYS_CLOSE:
    return sys_close(f.args[0]);
  default:
    return -1;
  }
}
