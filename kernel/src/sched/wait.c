#include "sched/wait.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"

void wait_on_queue(wait_queue_t *queue, spinlock_t *condition_lock) {
  thread_t *current = dispatch_get_current();

  // Protect wait queue manipulation
  spinlock_acquire(&queue->lock);

  // Add to wait queue
  current->state = THREAD_STATE_BLOCKED;
  current->next = NULL;
  if (!queue->tail) {
    queue->head = queue->tail = current;
  } else {
    queue->tail->next = current;
    queue->tail = current;
  }

  // Atomically release condition lock and switch
  // (while still holding queue->lock to prevent lost wakeup)
  if (condition_lock) {
    spinlock_release(condition_lock);
  }

  spinlock_release(&queue->lock);
  thread_t *next = sched_pick_next();
  dispatch_switch_to(next);

  // Re-acquire condition lock when woken up
  if (condition_lock) {
    spinlock_acquire(condition_lock);
  }
}

void wake_up_queue(wait_queue_t *queue) {
  spinlock_acquire(&queue->lock);

  if (queue->head) {
    thread_t *thread = queue->head;
    queue->head = thread->next;
    if (!queue->head) {
      queue->tail = NULL;
    }
    thread->next = NULL;
    thread->state = THREAD_STATE_READY;
    sched_enqueue(thread); // Has its own internal lock
  }

  spinlock_release(&queue->lock);
}
