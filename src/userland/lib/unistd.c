#include "unistd.h"
#include <stdio.h>

pid_t fork() {
  return (pid_t)_syscall6(SYSCALL_NUMBER_SYS_FORK, 0, 0, 0, 0, 0,
                          0); // Assigned 5 for SYS_FORK
}

int execve(const char *pathname, char *const argv[], char *const envp[]) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_EXECVE, pathname, argv, envp, 0, 0,
                        0);
}

void _exit(int status) {
  _syscall6(SYSCALL_NUMBER_SYS_EXIT, status, 0, 0, 0, 0, 0);
  while (1)
    ; // Should never reach here
}

void exit(int status) {
  fflush(NULL);
  _exit(status);
}

pid_t waitpid(pid_t pid, int *wstatus, int options) {
  return (pid_t)_syscall6(SYSCALL_NUMBER_SYS_WAITPID, pid, wstatus, options, 0,
                          0, 0);
}

// --- Memory Management ---

void *sbrk(intptr_t increment) {
  // Usually returns the previous break address
  return (void *)_syscall6(SYSCALL_NUMBER_SYS_SBRK, increment, 0, 0, 0, 0, 0);
}

// --- File/Device I/O ---

int open(const char *pathname, int flags) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_OPEN, pathname, flags, 0, 0, 0, 0);
}

int close(int fd) {
  return (int)_syscall6(SYSCALL_NUMBER_SYS_CLOSE, fd, 0, 0, 0, 0, 0);
}

ssize_t read(int fd, void *buf, size_t count) {
  return (ssize_t)_syscall6(SYSCALL_NUMBER_SYS_READ, fd, buf, count, 0, 0, 0);
}

ssize_t write(int fd, const void *buf, size_t count) {
  return (ssize_t)_syscall6(SYSCALL_NUMBER_SYS_WRITE, fd, buf, count, 0, 0, 0);
}

// --- Scheduling ---

unsigned int sleep(unsigned int seconds) {
  return (unsigned int)_syscall6(SYSCALL_NUMBER_SYS_SLEEP, seconds, 0, 0, 0, 0,
                                 0);
}

void yield(void) { _syscall6(SYSCALL_NUMBER_SYS_YIELD, 0, 0, 0, 0, 0, 0); }
