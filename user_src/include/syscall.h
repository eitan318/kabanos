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

#define _syscall6(num, a1, a2, a3, a4, a5, a6)                                 \
  __syscall6((num), (long)(a1), (long)(a2), (long)(a3), (long)(a4),            \
             (long)(a5), (long)(a6))

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
