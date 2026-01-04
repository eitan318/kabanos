#pragma once
#include "task.h"

TCB *scheduler_pick_next(void);
void scheduler_add(TCB *p);
void yield(void);
void test_tasks();

typedef struct {
  uint32_t prev;
  uint32_t esp0;
  uint32_t ss0;
  uint32_t unused[23];
} __attribute__((packed)) TSS;
