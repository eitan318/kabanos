#pragma once
#include "thread.h"

// Switches the CPU from the current thread to 'next'
// This performs the actual register swap
void dispatch_switch_to(thread_t *next);

// Special case: Starts the very first thread during OS boot
void dispatch_start_first(thread_t *first);

// Helper to get the thread currently owning the CPU
thread_t *dispatch_get_current(void);
