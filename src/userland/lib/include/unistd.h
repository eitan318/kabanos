#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#define DELAY_LOOP 10000000

typedef uint32_t pid_t;
typedef uint64_t ssize_t;

#define STDIN_FILENO 0  /* Standard input */
#define STDOUT_FILENO 1 /* Standard output */
#define STDERR_FILENO 2 /* Standard error */

FILE *fdopen(int fd, const char *mode);
off_t lseek(int fd, off_t offset, int whence);
int unlink(const char *pathname);
char *getcwd(char *buf, size_t size);
int execvp(const char *file, char *const argv[]);

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
