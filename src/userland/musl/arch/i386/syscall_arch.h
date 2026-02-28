
#define __SYSCALL_LL_E(x)                                                      \
  ((union {                                                                    \
    long long ll;                                                              \
    long l[2];                                                                 \
  }){.ll = x})                                                                 \
      .l[0],                                                                   \
      ((union {                                                                \
        long long ll;                                                          \
        long l[2];                                                             \
      }){.ll = x})                                                             \
          .l[1]
#define __SYSCALL_LL_O(x) __SYSCALL_LL_E((x))

#if SYSCALL_NO_TLS
#define SYSCALL_INSNS "int $128"
#else
#define SYSCALL_INSNS "call *%%gs:16"
#endif

#define SYSCALL_INSNS_12                                                       \
  "xchg %%ebx,%%edx ; " SYSCALL_INSNS " ; xchg %%ebx,%%edx"
#define SYSCALL_INSNS_34                                                       \
  "xchg %%ebx,%%edi ; " SYSCALL_INSNS " ; xchg %%ebx,%%edi"

static inline long __syscall6(long n, long a1, long a2, long a3, long a4,
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
                   : "0"(n), "b"(a1), "g"(a2), "g"(a3), "g"(a4), "g"(a5),
                     "g"(a6)
                   : "memory", "ecx", "edx");
  return ret;
}

static inline long __syscall0(long n) {
  return __syscall6(n, 0, 0, 0, 0, 0, 0);
}

static inline long __syscall1(long n, long a1) {
  return __syscall6(n, a1, 0, 0, 0, 0, 0);
}

static inline long __syscall2(long n, long a1, long a2) {
  return __syscall6(n, a1, a2, 0, 0, 0, 0);
}

static inline long __syscall3(long n, long a1, long a2, long a3) {
  return __syscall6(n, a1, a2, a3, 0, 0, 0);
}

static inline long __syscall4(long n, long a1, long a2, long a3, long a4) {
  return __syscall6(n, a1, a2, a3, 0, 0, 0);
}

static inline long __syscall5(long n, long a1, long a2, long a3, long a4,
                              long a5) {
  return __syscall6(n, a1, a2, a3, 0, 0, 0);
}

#define VDSO_USEFUL
#define VDSO_CGT32_SYM "__vdso_clock_gettime"
#define VDSO_CGT32_VER "LINUX_2.6"
#define VDSO_CGT_SYM "__vdso_clock_gettime64"
#define VDSO_CGT_VER "LINUX_2.6"
