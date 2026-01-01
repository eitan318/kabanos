#pragma once
#include "arch/i686/isr/isr.h"
#include <stdint.h>

#define KERNEL_CS 0x8
#define KERNEL_DS 0x10
#define PREEMPTIVE_INT 45

typedef struct Task {
  uint32_t *kernel_esp; // pointer to saved ISR stack
} Task;

typedef struct __attribute__((packed)) {
  uint32_t esp;
} PCB;

void setup_task(Task *t, void (*entry)(void), uint8_t *stack);
