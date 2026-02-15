#pragma once
#include "thread.h"

void dispatch_switch_preserve_context(void *context, thread_t *next);
void dispatch_switch_to(thread_t *next);
void dispatch_switch_first(thread_t *next);
thread_t *dispatch_get_current(void);
