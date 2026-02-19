#pragma once
#include "syscall.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define DELAY_LOOP 10000000

typedef uint32_t pid_t;
typedef uint64_t ssize_t;

typedef enum {
  FD_STDIN = 0,
  FD_STDOUT = 1,
  FD_STDERR = 2,
} STREAM_FD;

pid_t fork();
int execve(const char *pathname, char *const argv[], char *const envp[]);
void _exit(int status);
pid_t waitpid(pid_t pid, int *wstatus, int options);
void *sbrk(intptr_t increment);
int open(const char *pathname, int flags);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
unsigned int sleep(unsigned int seconds);
void yield(void);
