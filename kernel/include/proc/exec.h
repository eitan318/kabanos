#pragma once
#include "sched/thread.h"

int process_exec_noreturn(const char *path, enum thread_priority p);
int process_spawn(const char *path, enum thread_priority p);
long sys_execve(const char *pathname, char *const argv[], char *const envp[]);
