#include "schedualer.h"
#include "arch/i686/isr/isr.h"
#include "include/stdio.h"
#include "process/task.h"
#include <stddef.h>

static PCB *tasks[2];
static int task_count = 0;
static int current_index = 0;
static PCB kernel_task;
PCB *current = NULL;
static int next_pid = 1;

static uint32_t kernel_cr3; // physical address of kernel page directory
                            //
// uint32_t *create_page_directory(void) {
//   uint32_t *pd = kmalloc_aligned(4096, 4096); // 4KB aligned
//   memset(pd, 0, 4096);
//   // Map kernel space (e.g., higher half) identically
//   for (int i = KERNEL_START / PAGE_SIZE; i < 1024; i++) {
//     pd[i] = kernel_page_directory[i];
//   }
//   return pd;
// }

void scheduler_add(PCB *p) {
  p->pid = next_pid++;
  p->cr3 = (uint32_t)kernel_cr3;
  tasks[task_count++] = p;
}

PCB *scheduler_pick_next(void) {
  current_index = (current_index + 1) % task_count;
  return tasks[current_index];
}

extern void __attribute__((naked)) switch_to(PCB *p);

static void preemptie_switch_isr_handler(Registers *regs) {
  if (current == NULL) {
    // First time: kernel interrupted
    kernel_task.kernel_esp = (uint32_t *)regs;
    current = &kernel_task;
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

#define STACK_SIZE 4096

uint8_t stackA[STACK_SIZE];
uint8_t stackB[STACK_SIZE];

void test_tasks(uint32_t kernel_cr3_param) {
  kernel_cr3 = kernel_cr3_param;
  i686_isr_handler_register(PREEMPTIVE_INT, preemptie_switch_isr_handler);
  static PCB a, b;

  setup_task(&a, taskA, stackA);
  setup_task(&b, taskB, stackB);

  scheduler_add(&a);
  scheduler_add(&b);

  current = NULL;

  asm volatile("int $45");
}
