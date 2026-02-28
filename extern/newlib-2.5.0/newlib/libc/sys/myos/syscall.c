#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>

static inline long __syscall6(long num, long a1, long a2, long a3, long a4,
                              long a5, long a6) {
  long ret;
  __asm__ volatile("push %7\n\t"
                   "push %6\n\t"
                   "push %5\n\t"
                   "push %4\n\t"
                   "push %3\n\t"
                   "mov %%esp, %%ecx\n\t"
                   "lea 1f, %%edx\n\t"
                   "sysenter\n\t"
                   "1:\n\t"
                   "add $20, %%esp\n\t"
                   : "=a"(ret)
                   : "0"(num), "b"(a1), "g"(a2), "g"(a3), "g"(a4), "g"(a5),
                     "g"(a6)
                   : "memory", "ecx", "edx");
  return ret;
}

static int __myos_errno;
static inline long __syscall_ret(unsigned long r) {
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  if (r > -4096) {
    __myos_errno = -r;
    return -1;
  }
  return r;
}

#define _syscall6(num, a1, a2, a3, a4, a5, a6)                                 \
  __syscall_ret(__syscall6((num), (long)(a1), (long)(a2), (long)(a3),          \
                           (long)(a4), (long)(a5), (long)(a6)))

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

char **environ;

int isatty(int file) {
  if (file >= 0 && file <= 2)
    return 1;
  return 0;
}

int fork(void) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_FORK, 0, 0, 0, 0, 0, 0);
}

int execve(char *name, char **argv, char **env) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_EXECVE, (long)name, (long)argv,
                        (long)env, 0, 0, 0);
}

void _exit(int status) {
  _syscall6(SYSCALL_NUMBER_SYS_EXIT, status, 0, 0, 0, 0, 0);
  while (1)
    ;
}

int wait(int *status) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_WAITPID, -1, (long)status, 0, 0, 0,
                        0);
}

caddr_t sbrk(int incr) {
  return (caddr_t)_syscall6(SYSCALL_NUMBER_SYS_SBRK, incr, 0, 0, 0, 0, 0);
}

int open(const char *name, int flags, ...) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_OPEN, (long)name, flags, 0, 0, 0, 0);
}

int close(int file) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_CLOSE, file, 0, 0, 0, 0, 0);
}

int read(int file, char *ptr, int len) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_READ, file, (long)ptr, len, 0, 0, 0);
}

int zwrite(int file, char *ptr, int len) {
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  return (int)_syscall6(SYSCALL_NUMBER_SYS_WRITE, file, (long)ptr, len, 0, 0,
                        0);
}

int lseek(int file, int ptr, int dir) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_LSEEK, file, ptr, dir, 0, 0, 0);
}

int fstat(int file, struct stat *st) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_FSTAT, file, (long)st, 0, 0, 0, 0);
}

int stat(const char *file, struct stat *st) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_STAT, (long)file, (long)st, 0, 0, 0,
                        0);
}

int getpid(void) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_GETPID, 0, 0, 0, 0, 0, 0);
}

int kill(int pid, int sig) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_KILL, pid, sig, 0, 0, 0, 0);
}

int link(char *old, char *new) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_LINK, (long)old, (long)new, 0, 0, 0,
                        0);
}

int unlink(char *name) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_UNLINK, (long)name, 0, 0, 0, 0, 0);
}

clock_t times(struct tms *buf) {
  return (clock_t)_syscall6(SYSCALL_NUMBER_SYS_TIMES, (long)buf, 0, 0, 0, 0, 0);
}

unsigned int sleep(unsigned int seconds) {
  return (unsigned int)_syscall6(SYSCALL_NUMBER_SYS_SLEEP, seconds, 0, 0, 0, 0,
                                 0);
}

void yield(void) { _syscall6(SYSCALL_NUMBER_SYS_YIELD, 0, 0, 0, 0, 0, 0); }
