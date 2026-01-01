#pragma once
#include "task.h"

PCB *scheduler_pick_next(void);
void scheduler_add(PCB *p);
void yield(void);
void test_tasks(uint32_t kernel_cr3_param);
