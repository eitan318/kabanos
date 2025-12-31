#include "schedualer.h"
#include "arch/i686/isr/isr.h"
#include "include/stdio.h"
#include "process/task.h"
#include <stddef.h>
//
// #define PREEMPTIVE_INT 3
//
// void isr_handler(Registers *regs) {}
//
// i686_isr_handler_register(PREEMPTIVE_INT, isr_handler);

static Task *tasks[2];
static int task_count = 0;
static int current_index = 0;
Task *current = NULL;

void taskA(void) {
  while (1) {
    debugf_and_printf("A");
    yield();
  }
}

void taskB(void) {
  while (1) {
    debugf_and_printf("B");
    yield();
  }
}

#define STACK_SIZE 4096

uint8_t stackA[STACK_SIZE];
uint8_t stackB[STACK_SIZE];

void test_tasks(void) {
  static Task a, b;

  setup_task(&a, taskA, stackA);
  setup_task(&b, taskB, stackB);

  scheduler_add(&a);
  scheduler_add(&b);

  current = &a;

  // Start first task
  asm volatile("mov %0, %%esp\n"
               "popa\n\t"
               "ret\n"
               :
               : "r"(a.esp)
               : "memory");
}

void scheduler_add(Task *t) { tasks[task_count++] = t; }

Task *scheduler_pick_next(void) {
  current_index = (current_index + 1) % task_count;
  return tasks[current_index];
}

extern void __attribute__((naked)) switch_to(Task *current, Task *next);

void yield(void) {
  Task *next = scheduler_pick_next();
  if (next != current) {
    Task *prev = current;
    current = next;
    switch_to(prev, next);
  }
}
