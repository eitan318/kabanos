#include "sched/sched.h"
#include "hal.h"
#include "sched/thread.h"
#include <stddef.h>
#include <stdint.h>

extern vmspace_t *g_kernel_vmspace; // global

static thread_t *tasks[3];
static int task_count = 0;
static int current_index = 0;
static thread_t kernel_task;
thread_t *current = NULL;

void sched_add(thread_t *t) { tasks[task_count++] = t; }

static thread_t *sched_next(void) {
  current_index = (current_index + 1) % task_count;
  return tasks[current_index];
}

extern void __attribute__((naked)) switch_to(thread_t *p);

void sched_tick(struct regs *r) {
  if (current == NULL) {
    current = sched_next();
  } else {
    current->kernel_esp = (void *)r;
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

void sched_init(void) { current = NULL; }
