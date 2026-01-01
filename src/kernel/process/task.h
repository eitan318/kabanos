#pragma once
#include "arch/i686/isr/isr.h"
#include <stdint.h>

#define PREEMPTIVE_INT 45

// Default memory layout for processes

#define PROCESS_STACK_TOP 0xBFFFF000  // Just below 3GB
#define PROCESS_STACK_SIZE 0x00002000 // 8KB

typedef struct PCB {
  uint32_t pid;          // Process ID
  uint32_t *kernel_esp;  // pointer to saved ISR stack
  uint8_t *kernel_stack; // base
  uint32_t cr3;          // Page directory for this process (future)
  uint32_t *user_esp;    // Saved user stack pointer (we'll use soon)
} PCB;

void setup_pcb(PCB *t, void (*entry)(void));
