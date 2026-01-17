#include "sched/sched.h"
#include "arch/i686/gdt.h"
#include "arch/i686/isr/isr.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "include/string.h"
#include "memory_management/memdefs.h"
#include "memory_management/pmm.h"
#include "memory_management/va_allocation.h"
#include "memory_management/vmm.h"
#include "proc/exec.h"
#include "sched/thread.h"
#include <stddef.h>
#include <stdint.h>

extern vmspace_t *g_kernel_vmspace; // global

static thread_t *tasks[2];
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

static void preemptive_switch_isr_handler(Registers *regs) {
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
  TSSEntry *curr_tss = tss_entry_get(cpu_id);
  curr_tss->ss0 = i686_GDT_KERNEL_DS_SEL;
  curr_tss->esp0 = (uint32_t)(next->kstack);

  switch_to(next);
}

void sched_init() {
  i686_isr_handler_register(PREEMPTIVE_INT, preemptive_switch_isr_handler);
}
