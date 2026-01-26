#include "sched/sched.h"
#include "hal.h"
#include "isr.h"
#include "sched/thread.h"
#include <stddef.h>
#include <stdint.h>

#define TIMER_IRQ 0
#define TIMER_INTERRUPT 32

extern vmspace_t *g_kernel_vmspace; // global

static thread_t *tasks[3];
static int task_count = 0;
static int current_index = 0;
static thread_t kernel_task;
thread_t *current = NULL;

void sched_add(thread_t *t) { tasks[task_count++] = t; }

thread_t *sched_next(void) {
  current_index = (current_index + 1) % task_count;
  return tasks[current_index];
}

extern void __attribute__((naked)) switch_to(thread_t *p);
extern void __attribute__((naked)) switch_debug(thread_t *p);

static void preemptive_switch_isr_handler(struct regs *regs) {

  hal_irq_send_eoi(TIMER_IRQ);

  if (current == NULL) {
    current = sched_next();
  } else {
    current->kernel_esp = (void *)regs;
  }

  thread_t *next = sched_next();
  if (!next) {
    next = &kernel_task;
  }
  current = next;

  uint32_t cpu_id = 0;
  hal_set_kernel_stack(cpu_id, next->kstack_top);

  switch_to(next);
}

void sched_init() {
  // Enable keyboard interrupt (IRQ0)
  hal_irq_enable(TIMER_IRQ);
  isr_handler_register(TIMER_INTERRUPT, preemptive_switch_isr_handler);
}
