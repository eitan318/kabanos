#pragma once
#include <stdint.h>

typedef struct __attribute__((packed)) {
  uint32_t esp;
} Task;

typedef struct __attribute__((packed)) {
  uint32_t esp;
} PCB;

void setup_task(Task *t, void (*entry)(void), uint8_t *stack);
__attribute__((naked)) void switch_to(Task *current, Task *next);
