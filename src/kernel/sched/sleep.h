#pragma once
#include "thread.h"

void wake_up_sleeping(uint32_t g_curr_tick);
void enqueue_sleeper(thread_t *t, uint32_t wake_up_time);
