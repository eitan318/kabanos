/**
 * @file wait.h
 * @brief Wait queues: blocking threads until an event occurs.
 */
#pragma once
#include "sched/thread.h"
#include "spinlock.h"

/** @brief FIFO of threads blocked waiting for an event. */
typedef struct wait_queue {
  thread_t *head;
  thread_t *tail;
  spinlock_t lock;
} wait_queue_t;

/**
 * @brief Blocks the current thread on @p queue.
 *
 * @param condition_lock Optional lock protecting the awaited condition;
 *        released while sleeping and re-acquired before returning
 *        (condition-variable semantics). May be NULL.
 */
void wait_on_queue(wait_queue_t *queue, spinlock_t *condition_lock);

/** @brief Wakes the oldest thread blocked on @p queue, if any. */
void wake_up_queue(wait_queue_t *queue);
