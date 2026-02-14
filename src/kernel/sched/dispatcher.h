#pragma once
#include "thread.h"

void dispatch_init(thread_t *initial_task);

void dispatch_switch_preserve_context(void *context, thread_t *next);
void dispatch_switch_to(thread_t *next);
thread_t *dispatch_get_current(void);
