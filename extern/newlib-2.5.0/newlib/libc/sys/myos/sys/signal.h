#ifndef _SYS_SIGNAL_H_
#define _SYS_SIGNAL_H_

#include <sys/_types.h>

typedef void (*_sig_func_ptr)(int);
typedef unsigned long sigset_t;

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGFPE 8
#define SIGKILL 9
#define SIGBUS 10 /* bus error */
#define SIGSEGV 11
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15

/* FPE codes */
#define FPE_INTDIV 1
#define FPE_INTOVF 2
#define FPE_FLTDIV 3
#define FPE_FLTOVF 4
#define FPE_FLTUND 5
#define FPE_FLTRES 6
#define FPE_FLTINV 7
#define FPE_FLTSUB 8

/* SEGV codes */
#define SEGV_MAPERR 1
#define SEGV_ACCERR 2

/* Signal set management */
#define SIG_BLOCK 1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 3

typedef struct {
  int si_signo;
  int si_code;
  int si_errno;
} siginfo_t;

/* stack_t needed by ucontext */
typedef struct {
  void *ss_sp;
  int ss_flags;
  size_t ss_size;
} stack_t;

struct sigaction {
  union {
    _sig_func_ptr sa_handler;
    void (*sa_sigaction)(int, siginfo_t *, void *);
  } __sigaction_u;
  sigset_t sa_mask;
  int sa_flags;
};

#define sa_handler __sigaction_u.sa_handler
#define sa_sigaction __sigaction_u.sa_sigaction
#define SA_SIGINFO 0x0004

/* Prototypes */
int sigfillset(sigset_t *set);
int sigemptyset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigismember(const sigset_t *set, int signo);
int sigprocmask(int how, const sigset_t *set, sigset_t *oset);
int sigaction(int signum, const struct sigaction *act, struct sigaction *oact);

#endif /* _SYS_SIGNAL_H_ */
