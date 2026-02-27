#pragma once
#include "syscall.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define DELAY_LOOP 10000000

typedef uint32_t pid_t;
typedef uint64_t ssize_t;

#define STDIN_FILENO 0  /* Standard input */
#define STDOUT_FILENO 1 /* Standard output */
#define STDERR_FILENO 2 /* Standard error */

pid_t fork();
int execve(const char *pathname, char *const argv[], char *const envp[]);
void exit(int status);
pid_t waitpid(pid_t pid, int *wstatus, int options);
void *sbrk(intptr_t increment);
int open(const char *pathname, int flags);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
unsigned int sleep(unsigned int seconds);
void yield(void);
