#include "syscall.h"
#include "fs/fs_syscalls.h"
#include "proc/exec.h"
#include "proc/proc.h"
#include "sched/sched.h"
#include "sched/sleep.h"

typedef enum {
  /* --- File & Device I/O --- */
  SYSCALL_NUMBER_SYS_WRITE = 1,
  SYSCALL_NUMBER_SYS_READ = 4,
  SYSCALL_NUMBER_SYS_OPEN = 5,
  SYSCALL_NUMBER_SYS_CLOSE = 6,
  SYSCALL_NUMBER_SYS_LSEEK = 7,
  SYSCALL_NUMBER_SYS_STAT = 8,
  SYSCALL_NUMBER_SYS_FSTAT = 33,
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
  SYSCALL_NUMBER_SYS_LINK = 34,
  SYSCALL_NUMBER_SYS_TIMES = 35,
} SYSCALL_NUMBER;

long syscall_dispatch(syscall_info_t f) {
  switch (f.num) {
  /* --- File & Device I/O --- */
  case SYSCALL_NUMBER_SYS_OPEN:
    return sys_open((const char *)f.args[0], (int)f.args[1]);
  case SYSCALL_NUMBER_SYS_CLOSE:
    return sys_close((int)f.args[0]);
  case SYSCALL_NUMBER_SYS_READ:
    return sys_read((int)f.args[0], (char *)f.args[1], (size_t)f.args[2]);
  case SYSCALL_NUMBER_SYS_WRITE:
    return sys_write((int)f.args[0], (const char *)f.args[1],
                     (size_t)f.args[2]);
  case SYSCALL_NUMBER_SYS_LSEEK:
    return sys_lseek((int)f.args[0], (off_t)f.args[1], (int)f.args[2]);
  case SYSCALL_NUMBER_SYS_FSTAT:
  case SYSCALL_NUMBER_SYS_STAT:
    return sys_stat((int)f.args[0], (fstat_t *)f.args[1]);
  case SYSCALL_NUMBER_SYS_ITER_DIR:
    return sys_iter_dir((int)f.args[0], (VDirEntry *)f.args[1], (int)f.args[2]);

  /* --- Directory & Path Ops --- */
  case SYSCALL_NUMBER_SYS_MKDIR:
    return sys_mkdir((const char *)f.args[0], (mode_t)f.args[1]);
  case SYSCALL_NUMBER_SYS_RMDIR:
    return sys_rmdir((const char *)f.args[0]);
  case SYSCALL_NUMBER_SYS_UNLINK:
    return sys_unlink((const char *)f.args[0]);
  case SYSCALL_NUMBER_SYS_RENAME:
    return sys_rename((const char *)f.args[0], (const char *)f.args[1]);
  case SYSCALL_NUMBER_SYS_SYMLINK:
    return sys_symlink((const char *)f.args[0], (const char *)f.args[1]);
  case SYSCALL_NUMBER_SYS_READLINK:
    return sys_readlink((const char *)f.args[0], (char *)f.args[1],
                        (size_t)f.args[2]);
  case SYSCALL_NUMBER_SYS_LINK:
    return -ENOSYS; // Unimplemented

  /* --- Mount Management --- */
  case SYSCALL_NUMBER_SYS_MOUNT:
    return sys_mount((const char *)f.args[0], (const char *)f.args[1],
                     (const char *)f.args[2], (unsigned long)f.args[3],
                     (void *)f.args[4]);
  case SYSCALL_NUMBER_SYS_UMOUNT:
    return sys_umount((const char *)f.args[0]);

  /* --- Process Management --- */
  case SYSCALL_NUMBER_SYS_FORK:
    return sys_fork();
  case SYSCALL_NUMBER_SYS_EXECVE:
    return sys_execve((const char *)f.args[0], (char *const *)f.args[1],
                      (char *const *)f.args[2]);
  case SYSCALL_NUMBER_SYS_EXIT:
    sys_exit((int)f.args[0]);
    return 0;
  case SYSCALL_NUMBER_SYS_WAITPID:
    return sys_waitpid((pid_t)f.args[0], (int *)f.args[1], (int)f.args[2]);
  case SYSCALL_NUMBER_SYS_GETPID:
    return sys_getpid();

  /* --- Scheduling & Time --- */
  case SYSCALL_NUMBER_SYS_YIELD:
    sys_yield();
    return 0;
  case SYSCALL_NUMBER_SYS_SLEEP:
    sys_sleep((unsigned int)f.args[0]);
    return 0;
  case SYSCALL_NUMBER_SYS_NANOSLEEP:
  case SYSCALL_NUMBER_SYS_GETTIMEOFDAY:
  case SYSCALL_NUMBER_SYS_TIMES:
    return -ENOSYS; // Unimplemented

  /* --- Memory Management --- */
  case SYSCALL_NUMBER_SYS_SBRK:
  case SYSCALL_NUMBER_SYS_MMAP:
  case SYSCALL_NUMBER_SYS_MUNMAP:
    return -ENOSYS; // Unimplemented

  /* --- Signals & IPC --- */
  case SYSCALL_NUMBER_SYS_KILL:
  case SYSCALL_NUMBER_SYS_PIPE:
  case SYSCALL_NUMBER_SYS_SIGACTION:
    return -ENOSYS; // Unimplemented

  default:
    return -EINVAL;
  }
}
