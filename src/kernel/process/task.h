#pragma once
#include "arch/i686/isr/isr.h"
#include <stdint.h>

#define PREEMPTIVE_INT 45

#define KERNEL_STACK_SIZE PAGE_SIZE

typedef enum { TASK_MODE_KERNEL, TASK_MODE_USER } TaskMode;

typedef struct TCB {
  uint32_t pid;         // Process ID
  uint32_t *kernel_esp; // pointer to saved ISR stack
  uint32_t *user_esp;   // Saved user stack pointer (we'll use soon)
  uint32_t cr3;         // Page directory for this process (future)
  uint32_t *kernel_stack_top;
  TaskMode mode;
} __attribute__((packed)) TCB;

void task_setup(TCB *t, void (*entry)(void));
