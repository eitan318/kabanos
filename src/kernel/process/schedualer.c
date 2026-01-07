#include "schedualer.h"
#include "arch/i686/gdt.h"
#include "arch/i686/isr/isr.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "memory_management/pmm.h"
#include "memory_management/va_allocation.h"
#include "memory_management/vmm.h"
#include "process/task.h"
#include <stddef.h>

extern vmspace_t *g_kernel_vmspace; // global

static TCB *tasks[2];
static int task_count = 0;
static int current_index = 0;
static TCB kernel_task;
TCB *current = NULL;

void scheduler_add(TCB *t) { tasks[task_count++] = t; }

TCB *scheduler_pick_next(void) {
  current_index = (current_index + 1) % task_count;
  return tasks[current_index];
}

extern void __attribute__((naked)) switch_to(TCB *p);
extern void __attribute__((naked)) switch_debug(TCB *p);

static void preemptive_switch_isr_handler(Registers *regs) {
  if (current == NULL) {
    current = scheduler_pick_next();
  } else {
    current->kernel_esp = (uint32_t *)regs;
  }

  TCB *next = scheduler_pick_next();
  if (!next) {
    next = &kernel_task;
  }
  current = next;

  uint32_t cpu_id = 0;
  TSSEntry *curr_tss = tss_entry_get(cpu_id);
  curr_tss->ss0 = i686_GDT_KERNEL_DS_SEL;
  curr_tss->esp0 = (uint32_t)(next->kernel_stack_top);

  switch_to(next);
}

void taskA(void) {
  while (1) {
    debugf_and_printf("Task A PID: %d\n", current->pid);
    asm volatile("int $45");
  }
}

void taskB(void) {
  while (1) {
    debugf_and_printf("Task B PID: %d\n", current->pid);
    asm volatile("int $45");
  }
}

void test_tasks() {
  i686_isr_handler_register(PREEMPTIVE_INT, preemptive_switch_isr_handler);

  // TCB *a = task_setup(taskA, TASK_MODE_USER);
  // TCB *b = task_setup(taskB, TASK_MODE_KERNEL);
  // if (a == NULL || a == NULL) {
  //   debugf("TASK Were null!!! ERR");
  //   return;
  // }
  //
  TCB a, b;
  task_setup(&a, taskA, TASK_MODE_KERNEL);
  task_setup(&b, taskB, TASK_MODE_KERNEL);
  //
  // TCB *c = task_setup(taskA, TASK_MODE_USER);
  // TCB *d = task_setup(taskB, TASK_MODE_KERNEL);
  //
  scheduler_add(&a);
  scheduler_add(&b);

  current = NULL;

  asm volatile("int $45");
}
