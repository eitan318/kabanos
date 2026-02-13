#pragma once
#include "thread.h"

void dispatch_init();
void dispatch_switch_from_interrupt(void *context, thread_t *next);
void dispatch_switch_from_kernel(thread_t *next);

// Special case: Starts the very first thread during OS boot
void dispatch_start_first(thread_t *first);

// Helper to get the thread currently owning the CPU
thread_t *dispatch_get_current(void);
