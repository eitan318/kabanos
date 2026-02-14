#include "dispatcher.h"
#include "hal.h"
#include "isr.h"

thread_t *g_current_thread = NULL;
static thread_t *pending_switch_target = NULL;

void dispatch_switch_preserve_context(void *context, thread_t *next) {
  thread_t *current = dispatch_get_current();

  if (!next || current == next) {
    return;
  }

  // Save current thread context
  hal_thread_save_context(current->arch, context);
  hal_update_tss_and_syssenter_kstack(0, next->kstack_top);

  next->state = THREAD_RUNNING;
  g_current_thread = next;

  hal_thread_switch(next);
}

void dispatch_switch_to(thread_t *next) {
  pending_switch_target = next;
  __asm__ volatile("int $0x81");
}

static void handle_voluntary_yield(arch_regs *context) {
  dispatch_switch_preserve_context(context, pending_switch_target);
}

void dispatch_init(thread_t *initial_task) {
  g_current_thread = initial_task;
  isr_handler_register(0x81, handle_voluntary_yield);
}

thread_t *dispatch_get_current(void) { return g_current_thread; }
