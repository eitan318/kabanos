#include <stddef.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/unistd.h>

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
  SYSCALL_NUMBER_INITIAL = 1,

  // --- File System ---
  SYSCALL_NUMBER_SYS_WRITE,
  SYSCALL_NUMBER_SYS_READ,
  SYSCALL_NUMBER_SYS_OPEN,
  SYSCALL_NUMBER_SYS_CLOSE,
  SYSCALL_NUMBER_SYS_LSEEK,
  SYSCALL_NUMBER_SYS_FSTAT,
  SYSCALL_NUMBER_SYS_STAT, // Added: Used by stat()
  SYSCALL_NUMBER_SYS_GETDENTS,
  SYSCALL_NUMBER_SYS_CREATE,
  SYSCALL_NUMBER_SYS_UNLINK,
  SYSCALL_NUMBER_SYS_RENAME,
  SYSCALL_NUMBER_SYS_MKDIR,
  SYSCALL_NUMBER_SYS_RMDIR,
  SYSCALL_NUMBER_SYS_SYMLINK,
  SYSCALL_NUMBER_SYS_READLINK,
  SYSCALL_NUMBER_SYS_LINK, // Added: Used by link()
  SYSCALL_NUMBER_SYS_MOUNT,
  SYSCALL_NUMBER_SYS_UMOUNT,
  SYSCALL_NUMBER_SYS_GETCWD, // Added: Used by getcwd()

  // --- Process & Lifecycle ---
  SYSCALL_NUMBER_SYS_FORK,
  SYSCALL_NUMBER_SYS_EXECVE,
  SYSCALL_NUMBER_SYS_EXIT,
  SYSCALL_NUMBER_SYS_WAITPID,
  SYSCALL_NUMBER_SYS_GETPID,

  // --- Scheduling & Time ---
  SYSCALL_NUMBER_SYS_YIELD,
  SYSCALL_NUMBER_SYS_SLEEP,
  SYSCALL_NUMBER_SYS_NANOSLEEP,
  SYSCALL_NUMBER_SYS_GETTIMEOFDAY,
  SYSCALL_NUMBER_SYS_TIMES,

  // --- Memory Management ---
  SYSCALL_NUMBER_SYS_SBRK,
  SYSCALL_NUMBER_SYS_MMAP,
  SYSCALL_NUMBER_SYS_MUNMAP,
  SYSCALL_NUMBER_SYS_MPROTECT,

  // --- Signals & IPC ---
  SYSCALL_NUMBER_SYS_PIPE,
  SYSCALL_NUMBER_SYS_SIGACTION,
  SYSCALL_NUMBER_SYS_SIGPROCMASK,
  SYSCALL_NUMBER_SYS_KILL,

} SYSCALL_NUMBER;

/* getcwd */

char *getcwd(char *__buf, size_t __size) {
  long ret = _syscall6(SYSCALL_NUMBER_SYS_GETCWD, (long)__buf, (long)__size, 0,
                       0, 0, 0);
  if (ret < 0)
    return NULL;
  return __buf;
}

/* realpath - simple implementation using getcwd */
char *realpath(const char *path, char *resolved) {
  static char buf[4096];
  if (!resolved)
    resolved = buf;
  /* If absolute path, just copy it */
  if (path[0] == '/') {
    int i = 0;
    while (path[i] && i < 4095) {
      resolved[i] = path[i];
      i++;
    }
    resolved[i] = '\0';
    return resolved;
  }
  /* Relative: prepend cwd */
  if (!getcwd(resolved, 4096))
    return NULL;
  int len = 0;
  while (resolved[len])
    len++;
  resolved[len++] = '/';
  int i = 0;
  while (path[i] && len < 4095) {
    resolved[len++] = path[i++];
  }
  resolved[len] = '\0';
  return resolved;
}

/* sysconf */
long sysconf(int __name) {
  switch (__name) {
  case _SC_PAGESIZE:
    return 4096;
  case _SC_PHYS_PAGES:
    return 1024;
  default:
    return -1;
  }
}

/* mprotect */
int mprotect(void *addr, size_t len, int prot) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_MPROTECT, (long)addr, (long)len,
                        prot, 0, 0, 0);
}

/* execvp - search PATH and call execve */

int execvp(const char *__file, char *const __argv[]) {
  return execve((char *)__file, (char **)__argv, environ);
}

/* ldexpl stub - TCC uses this for float parsing */
long double ldexpl(long double x, int exp) {
  while (exp > 0) {
    x *= 2.0L;
    exp--;
  }
  while (exp < 0) {
    x /= 2.0L;
    exp++;
  }
  return x;
}

/* signal set management */
int sigemptyset(sigset_t *set) {
  *set = 0;
  return 0;
}

int sigfillset(sigset_t *set) {
  *set = ~(sigset_t)0;
  return 0;
}

int sigaddset(sigset_t *set, int signo) {
  *set |= (1UL << (signo - 1));
  return 0;
}

int sigdelset(sigset_t *set, int signo) {
  *set &= ~(1UL << (signo - 1));
  return 0;
}

int sigismember(const sigset_t *set, int signo) {
  return (*set & (1UL << (signo - 1))) != 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_SIGPROCMASK, how, (long)set,
                        (long)oset, 0, 0, 0);
}

int isatty(int file) {
  if (file >= 0 && file <= 2)
    return 1;
  return 0;
}

int fork(void) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_FORK, 0, 0, 0, 0, 0, 0);
}

int execve(const char *__path, char *const __argv[], char *const __envp[]) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_EXECVE, (long)__path, (long)__argv,
                        (long)__envp, 0, 0, 0);
}

void _exit(int __status) {
  _syscall6(SYSCALL_NUMBER_SYS_EXIT, __status, 0, 0, 0, 0, 0);
  while (1)
    ;
}

int waitpid(pid_t pid, int *status, int options) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_WAITPID, (long)pid, (long)status,
                        (long)options, 0, 0, 0);
}

// You can now keep wait() as a helper function that calls waitpid
int wait(int *status) { return waitpid(-1, status, 0); }

void *sbrk(ptrdiff_t __incr) {
  return (caddr_t)_syscall6(SYSCALL_NUMBER_SYS_SBRK, __incr, 0, 0, 0, 0, 0);
}

int open(const char *name, int flags, ...) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_OPEN, (long)name, flags, 0, 0, 0, 0);
}

int _close(int __fildes) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_CLOSE, __fildes, 0, 0, 0, 0, 0);
}

_READ_WRITE_RETURN_TYPE read(int __fd, void *__buf, size_t __nbyte) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_READ, __fd, (long)__buf, __nbyte, 0,
                        0, 0);
}

_READ_WRITE_RETURN_TYPE write(int __fd, const void *__buf, size_t __nbyte) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_WRITE, __fd, (long)__buf, __nbyte, 0,
                        0, 0);
}

off_t lseek(int __fildes, off_t __offset, int __whence) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_LSEEK, __fildes, __offset, __whence,
                        0, 0, 0);
}

int fstat(int file, struct stat *st) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_FSTAT, file, (long)st, 0, 0, 0, 0);
}

int stat(const char *file, struct stat *st) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_STAT, (long)file, (long)st, 0, 0, 0,
                        0);
}

pid_t getpid(void) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_GETPID, 0, 0, 0, 0, 0, 0);
}

int kill(int pid, int sig) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_KILL, pid, sig, 0, 0, 0, 0);
}

int link(const char *__path1, const char *__path2) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_LINK, (long)__path1, (long)__path2,
                        0, 0, 0, 0);
}

int unlink(const char *__path) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_UNLINK, (long)__path, 0, 0, 0, 0, 0);
}

clock_t times(struct tms *buf) {
  return (clock_t)_syscall6(SYSCALL_NUMBER_SYS_TIMES, (long)buf, 0, 0, 0, 0, 0);
}

unsigned sleep(unsigned int __seconds) {
  return (unsigned int)_syscall6(SYSCALL_NUMBER_SYS_SLEEP, __seconds, 0, 0, 0,
                                 0, 0);
}

void yield(void) { _syscall6(SYSCALL_NUMBER_SYS_YIELD, 0, 0, 0, 0, 0, 0); }

int sigaction(int signum, const struct sigaction *act, struct sigaction *oact) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_SIGACTION, signum, (long)act,
                        (long)oact, 0, 0, 0);
}

int getdents(int fd, void *buf, unsigned int size) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_GETDENTS, (long)fd, (long)buf,
                        (long)size, 0, 0, 0);
}

#include "dirent.h"
#include "malloc.h"

int closedir(DIR *dir) {
  close(dir->fd);

  free(dir);
}

DIR *opendir(const char *path) {

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    printf("small fd\n");
    return NULL;
  }

  printf("opendir: opening\n");

  DIR *dir = malloc(sizeof(DIR));
  if (!dir) {
    printf("malloc faild\n");
    close(fd);
    return NULL;
  }

  dir->fd = fd;
  dir->buf_pos = 0;
  dir->buf_end = 0;

  return dir;
}

extern int getdents(int fd, void *buf, unsigned int size);

struct dirent *readdir(DIR *dir) {
  if (dir->buf_pos >= dir->buf_end) {

    int n = getdents(dir->fd, dir->buffer, sizeof(dir->buffer));

    if (n <= 0)
      return NULL;

    dir->buf_pos = 0;
    dir->buf_end = n;
  }

  struct dirent *d = (struct dirent *)(dir->buffer + dir->buf_pos);

  dir->buf_pos += sizeof(struct dirent);

  memcpy(&dir->current, d, sizeof(struct dirent));

  return &dir->current;
}
