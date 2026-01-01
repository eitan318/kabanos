#include "schedualer.h"
#include "arch/i686/isr/isr.h"
#include "include/stdio.h"
#include "process/task.h"
#include <stddef.h>

static Task *tasks[2];
static int task_count = 0;
static int current_index = 0;
static Task kernel_task;
Task *current = NULL;

void scheduler_add(Task *t) { tasks[task_count++] = t; }

Task *scheduler_pick_next(void) {
  current_index = (current_index + 1) % task_count;
  return tasks[current_index];
}

extern void __attribute__((naked)) switch_to(Registers *regs);

static void preemptie_switch_isr_handler(Registers *regs) {
  if (current == NULL) {
    // First time: kernel interrupted
    kernel_task.kernel_esp = (uint32_t *)regs;
    current = &kernel_task;
  } else {
    // Save current task context
    current->kernel_esp = (uint32_t *)regs;
  }

  Task *next = scheduler_pick_next();
  if (!next) {
    next = &kernel_task;
  }
  current = next;

  switch_to(next->kernel_esp);
}

void taskA(void) {
  while (1) {
    debugf_and_printf("A");
    asm volatile("int $45");
  }
}

void taskB(void) {
  while (1) {
    debugf_and_printf("B");
    asm volatile("int $45");
  }
}

#define STACK_SIZE 4096

uint8_t stackA[STACK_SIZE];
uint8_t stackB[STACK_SIZE];

void test_tasks(void) {
  i686_isr_handler_register(PREEMPTIVE_INT, preemptie_switch_isr_handler);
  static Task a, b;

  setup_task(&a, taskA, stackA);
  setup_task(&b, taskB, stackB);

  scheduler_add(&a);
  scheduler_add(&b);

  current = NULL;

  asm volatile("int $45");
}
