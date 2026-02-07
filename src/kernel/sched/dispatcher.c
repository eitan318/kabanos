#include "dispatcher.h"
#include "hal.h"

thread_t *g_current_thread = NULL;

void dispatch_switch_to(thread_t *next) {
  if (!next || g_current_thread == next) {
    return;
  }

  hal_update_kernel_stack(0, next->kstack_top);

  thread_t *prev = g_current_thread;
  g_current_thread = next;
  next->state = THREAD_RUNNING;
  hal_thread_switch(next);
}

void dispatch_start_first(thread_t *first) {
  if (!first)
    return;

  g_current_thread = first;
  first->state = THREAD_RUNNING;

  hal_update_kernel_stack(0, first->kstack_top);
  hal_thread_switch(first);
}

thread_t *dispatch_get_current(void) { return g_current_thread; }
