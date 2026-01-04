#pragma once
#include "task.h"

TCB *scheduler_pick_next(void);
void scheduler_add(TCB *p);
void yield(void);
void test_tasks();
