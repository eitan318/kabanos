#include "sched/sched.h"
#include "hal.h"
#include "isr.h"
#include "sched/thread.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

extern vmspace_t *g_kernel_vmspace; // global

static thread_t *tasks[10];
static int task_count = 0;
static int current_index = 0;
static thread_t kernel_task;
thread_t *g_current_thread = NULL; // will be replaced in a multicore setup

void sched_add(thread_t *t) { tasks[task_count++] = t; }

static thread_t *sched_next(void) {
  current_index = (current_index + 1) % task_count;
  return tasks[current_index];
}

void debug_ticker(struct arch_regs *r) { sched_tick(r); }

void sched_tick(void *context) {
  if (g_current_thread != NULL) {
    hal_thread_save(g_current_thread->arch, context);
  }

  thread_t *next = sched_next();
  if (!next) {
    next = &kernel_task;
  }
  g_current_thread = next;

  int cpu_id = 0;
  hal_update_kernel_stack(cpu_id, next->kstack_top);
  hal_thread_switch(next);
}

void sched_init(void) {
  isr_handler_register(0x45, debug_ticker);
  g_current_thread = NULL;
}
