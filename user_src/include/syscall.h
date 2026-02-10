static inline long __syscall6(long num, long a1, long a2, long a3, long a4,
                              long a5, long a6) {
  long ret;
  // We use "g" for args that can be in memory to let the compiler
  // decide how to handle the register pressure.
  __asm__ volatile(
      "push %6\n\t"          // Push Arg 6 ([esp+8])
      "push %3\n\t"          // Push Arg 3 ([esp+4])
      "push %2\n\t"          // Push Arg 2 ([esp])
      "mov %%esp, %%ecx\n\t" // 1. ECX = User ESP (Required for transition)
      "lea 1f, %%edx\n\t" // 2. EDX = Return Address (Required for transition)
      "sysenter\n\t"
      "1:\n\t"             // The point where SYSEXIT lands
      "add $12, %%esp\n\t" // Clean up the 3 arguments we pushed
      : "=a"(ret)
      : "a"(num), "m"(a2), "m"(a3), "b"(a1), "S"(a4), "D"(a5), "m"(a6)
      : "memory", "ecx", "edx");
  return ret;
}

#define _syscall6(num, a1, a2, a3, a4, a5, a6)                                 \
  __syscall6((num), (long)(a1), (long)(a2), (long)(a3), (long)(a4),            \
             (long)(a5), (long)(a6))

typedef enum {
  SYSCALL_NUMBERS_SYS_WRITE = 1,
  SYSCALL_NUMBERS_SYS_YIELD = 2, // Added for FCFS rotation
  SYSCALL_NUMBERS_SYS_SLEEP = 3, // Added for Blocking I/O demo
  SYSCALL_NUMBERS_SYS_READ = 4   // Added for Blocking I/O demo
} SYSCALL_NUMBERS;

typedef enum {
  DEVICE_HANDLE_KEYBOARD = 1,
  DEVICE_HANDLE_ATA = 2,
} device_handle_t;
