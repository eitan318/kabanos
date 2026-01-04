#include "schedualer.h"
#include "arch/i686/gdt.h"
#include "arch/i686/isr/isr.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"
#include "memory_management/va_allocation.h"
#include "process/task.h"
#include <stddef.h>

#define KERNEL_VA_START 0xC0400000
#define KERNEL_VA_END 0xC0800000

extern PageDirectory *g_kernel_page_dir; // global
extern TSS g_tss;                        // global

static TCB *tasks[2];
static int task_count = 0;
static int current_index = 0;
static TCB kernel_task;
TCB *current = NULL;
static int next_pid = 1;

void scheduler_add(TCB *t) {
  t->pid = next_pid++;

  tasks[task_count++] = t;
}

TCB *scheduler_pick_next(void) {
  current_index = (current_index + 1) % task_count;
  return tasks[current_index];
}

extern void __attribute__((naked)) switch_to(TCB *p);

static void preemptive_switch_isr_handler(Registers *regs) {
  if (current == NULL) {
    // First time: kernel interrupted
    kernel_task.kernel_esp = (uint32_t *)regs;
    current = &kernel_task;
    debugf("First ISR: kernel task active\n");
  } else {
    // Save current task context
    current->kernel_esp = (uint32_t *)regs;
  }

  TCB *next = scheduler_pick_next();
  if (!next) {
    next = &kernel_task;
  }
  current = next;

  // Next or curr?
  g_tss.esp0 = (uint32_t)(next->kernel_esp);

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

uint8_t stackA[KERNEL_STACK_SIZE];
uint8_t stackB[KERNEL_STACK_SIZE];

void tss_init(void) {
  memset(&g_tss, 0, sizeof(TSS));
  g_tss.ss0 = i686_GDT_KERNEL_DATA_SEGMENT;
}

void test_tasks() {
  i686_isr_handler_register(PREEMPTIVE_INT, preemptive_switch_isr_handler);

  va_allocator_init(KERNEL_VA_START, KERNEL_VA_END, g_kernel_page_dir);
  static TCB a, b;

  setup_task(&a, taskA);
  setup_task(&b, taskB);

  scheduler_add(&a);
  scheduler_add(&b);

  a.mode = TASK_MODE_USER;
  b.mode = TASK_MODE_KERNEL;

  current = NULL;

  asm volatile("int $45");
}
