#pragma once
#include "task.h"

Task *scheduler_pick_next(void);
void scheduler_add(Task *t);
void yield(void);
