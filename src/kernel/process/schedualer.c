#include "schedualer.h"
#include "arch/i686/isr/isr.h"
#include "include/stdio.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"
#include "memory_management/va_allocation.h"
#include "process/task.h"
#include <stddef.h>

#define KERNEL_VA_START 0xC0400000
#define KERNEL_VA_END 0xC0800000

extern PageDirectory *g_kernel_page_dir; // global

static PCB *tasks[2];
static int task_count = 0;
static int current_index = 0;
static PCB kernel_task;
PCB *current = NULL;
static int next_pid = 1;

static uint32_t kernel_cr3; // physical address of kernel page directory

void scheduler_add(PCB *p) {
  p->pid = next_pid++;
  tasks[task_count++] = p;
}

PCB *scheduler_pick_next(void) {
  current_index = (current_index + 1) % task_count;
  return tasks[current_index];
}

extern void __attribute__((naked)) switch_to(PCB *p);

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

  PCB *next = scheduler_pick_next();
  if (!next) {
    next = &kernel_task;
  }
  current = next;

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
  kernel_cr3 = (uint32_t)g_kernel_page_dir;
  i686_isr_handler_register(PREEMPTIVE_INT, preemptive_switch_isr_handler);

  va_allocator_init(KERNEL_VA_START, KERNEL_VA_END, g_kernel_page_dir);
  static PCB a, b;

  setup_pcb(&a, taskA);
  setup_pcb(&b, taskB);

  scheduler_add(&a);
  scheduler_add(&b);

  current = NULL;

  asm volatile("int $45");
}
