#pragma once
#include "klib/time.h"
#include "sched/thread.h"

void wake_up_sleeping(uint32_t g_curr_tick);
void enqueue_sleeper(thread_t *t, uint32_t wake_up_time);
void sys_sleep(uint32_t seconds);
int sys_nanosleep(const timespec_t *req, timespec_t *rem);
