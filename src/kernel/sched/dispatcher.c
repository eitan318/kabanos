#include "dispatcher.h"
#include "hal.h"
#include "isr.h"

thread_t *g_current_thread = NULL;
thread_t *pending_switch_target = NULL;

void dispatch_switch_from_interrupt(void *context, thread_t *next) {
  thread_t *current = dispatch_get_current();

  if (!next || current == next) {
    return;
  }

  // Save current thread context
  hal_thread_save_context(current->arch, context);
  hal_update_tss_and_syssenter_kstack(0, next->kstack_top);

  // Update current thread pointer
  g_current_thread = next;
  next->state = THREAD_RUNNING;

  // Switch (never returns)
  hal_thread_switch(next);
}

void dispatch_switch_from_kernel(thread_t *next) {
  pending_switch_target = next;
  __asm__ volatile("int $0x81");
}

void handle_voluntary_yield(arch_regs *context) {
  dispatch_switch_from_interrupt(context, pending_switch_target);
}

void dispatch_init() { isr_handler_register(0x81, handle_voluntary_yield); }

void dispatch_start_first(thread_t *first) {
  if (!first)
    return;
  g_current_thread = first;
  first->state = THREAD_RUNNING;
  hal_update_tss_and_syssenter_kstack(0, first->kstack_top);
  hal_thread_switch(first);
}

thread_t *dispatch_get_current(void) { return g_current_thread; }
