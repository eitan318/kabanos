#pragma once
#include "arch/i686/isr/isr.h"
#include <stdint.h>

#define KERNEL_CS 0x8
#define KERNEL_DS 0x10
#define PREEMPTIVE_INT 45

typedef struct PCB {
  uint32_t pid;         // Process ID
  uint32_t *kernel_esp; // pointer to saved ISR stack
  uint32_t *user_esp;   // Saved user stack pointer (we'll use soon)
  uint32_t cr3;         // Page directory for this process (future)
} PCB;

void setup_task(PCB *t, void (*entry)(void), uint8_t *stack);
