#include "schedualer.h"
#include "include/stdio.h"
#include <stddef.h>

static Task *tasks[2];
static int task_count = 0;
static int current_index = 0;
Task *current = NULL;

void taskA(void) {
  while (1) {
    debugf("A");
    yield();
  }
}

void taskB(void) {
  while (1) {
    debugf("B");
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

void yield(void) {
  Task *next = scheduler_pick_next();
  if (next != current) {
    switch_to(current, next);
  }
}
