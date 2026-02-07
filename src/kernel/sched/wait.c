#include "wait.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "thread.h"
void wait_on_queue(wait_queue_t *queue) {
  // 1. Acquire the queue lock
  // This should also disable local interrupts to prevent deadlocks
  spinlock_acquire(&queue->lock);

  thread_t *current = dispatch_get_current();
  current->state = THREAD_BLOCKED;

  // 2. Add to the tail of the device-specific list
  if (!queue->tail) {
    queue->head = current;
    queue->tail = current;
  } else {
    queue->tail->next = current;
    queue->tail = current;
  }

  // 3. Release the lock BEFORE switching
  // NOTE: In a professional kernel, you often switch with the lock held
  // and have the NEXT thread release it, but for FCFS, release here is fine
  // if you handle the 'interrupts' state correctly.
  spinlock_release(&queue->lock);

  // 4. Switch to the next READY thread
  dispatch_switch_to(sched_pick_next());
}

void wake_up_queue(wait_queue_t *queue) {
  spinlock_acquire(&queue->lock);

  thread_t *t = queue->head;
  if (t) {
    // Remove the first thread in the waiting room
    queue->head = t->next;
    if (!queue->head) {
      queue->tail = NULL;
    }

    t->next = NULL;

    // Hand the thread back to the global READY queue
    // This function (sched_enqueue) has its OWN lock internally
    sched_enqueue(t);
  }

  spinlock_release(&queue->lock);
}
