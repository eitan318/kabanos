#include "dispatcher.h"
#include "hal.h"
#include "memory_management/memdefs.h"
#include "stdio.h"
#include "string.h"

thread_t *g_current_thread = NULL;

static thread_t kmain_thread;
static arch_thread_t kmain_arch;

extern uint8_t stack_bottom[BOOT_STACK_SIZE];

int dispatch_init(module_t *self) {
  memset(&kmain_thread, 0, sizeof(thread_t));

  kmain_thread.tid = 0; // The first thread
  kmain_thread.priority = PRIORITY_HIGH;
  kmain_thread.state = THREAD_RUNNING;
  kmain_thread.mode = THREAD_MODE_KERNEL;

  // Crucial: Point it to its arch-specific storage
  kmain_thread.arch = &kmain_arch;

  // We don't need to 'alloc_kernel_stack' because we are
  // already using the boot stack. Just point to the top of it.
  // (Ensure BOOT_STACK_TOP is the address from your linker/assembly)
  kmain_thread.kstack_top = (void *)(stack_bottom + BOOT_STACK_SIZE);
  kmain_thread.arch->kernel_esp = (void *)(stack_bottom + BOOT_STACK_SIZE);

  // Adopt this as the current thread
  g_current_thread = &kmain_thread;
  return 0;
}

void dispatch_switch_to(thread_t *next) {
  thread_t *current = dispatch_get_current();
  // printf("curr: %d, next %d", current->tid, next->tid);

  next->state = THREAD_RUNNING;
  if (!next || current == next) {
    return;
  }

  hal_update_tss_and_syssenter_kstack(0, next->kstack_top);

  g_current_thread = next;

  printf("NEXT (tid %d) esp %p", next->tid, next->arch->kernel_esp);
  hal_thread_switch(current, next);
}

// if g_current_thread = null ret IDLE task i think
thread_t *dispatch_get_current(void) { return g_current_thread; }

static const char *dispatch_deps[] = {"hal", NULL};

ITER_MODULE(dispatcher) = {
    .name = "dispatcher",
    .required = dispatch_deps,
    .init = &dispatch_init,
    .fini = NULL,
};
