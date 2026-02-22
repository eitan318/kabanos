#pragma once
#include "sched/thread.h"

int process_exec_noreturn(const char *path, enum thread_priority p);
int process_spawn(const char *path, enum thread_priority p);
