#include "sched/sched.h"
#include "arch/types.h"
#include "hal.h"
#include "isr.h"
#include "sched/thread.h"
#include "stdio.h"
#include <stddef.h>
#include <stdint.h>

extern vmspace_t *g_kernel_vmspace; // global

static thread_t *tasks[4];
static int task_count = 0;
static int current_index = 0;
static thread_t kernel_task;
thread_t *current = NULL;

void sched_add(thread_t *t) { tasks[task_count++] = t; }

static thread_t *sched_next(void) {
  current_index = (current_index + 1) % task_count;
  return tasks[current_index];
}

// Generic Kernel Code
void sched_yield() {
  thread_t *old = current;
  thread_t *next = sched_next();
  debugf("SWITCH: %d -> %d\n", old->tid, next->tid);

  current = next; // Make sure 'current' is updated before the jump!
  hal_vm_arch_load(next->process->vmspace->arch);
  hal_thread_switch(next->arch);
}

void ticker(struct arch_regs *r) { sched_tick(r); }

void sched_tick(void *context) {
  hal_interrupts_disable();
  if (current != NULL) {
    hal_thread_save(current->arch, context);
  }

  // 2. Pick the next thread
  thread_t *next = sched_next();
  if (!next) {
    next = &kernel_task;
  }
  current = next;

  int cpu_id = 0;
  hal_set_kernel_stack(cpu_id, next->kstack_top);
  hal_vm_arch_load(next->process->vmspace->arch);
  hal_interrupts_enable();
  hal_thread_switch(next->arch);
}

void sched_init(void) {
  isr_handler_register(0x45, ticker);
  current = NULL;
}
