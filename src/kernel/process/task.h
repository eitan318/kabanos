#pragma once
#include <stdint.h>

typedef struct {
  uint32_t esp;
} Task;

void setup_task(Task *t, void (*entry)(void), uint8_t *stack);

void switch_to(Task *current, Task *next);
