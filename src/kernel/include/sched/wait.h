#pragma once
#include "sched/thread.h"
#include "spinlock.h"

typedef struct wait_queue {
  thread_t *head;
  thread_t *tail;
  spinlock_t lock;
} wait_queue_t;

void wait_on_queue(wait_queue_t *queue, spinlock_t *condition_lock);
void wake_up_queue(wait_queue_t *queue);
