#include "sched.h"
#include "dispatcher.h"
#include "memory_management/kmalloc.h"
#include "sched/thread.h"

static thread_t *ready_queue_head = NULL;
static thread_t *ready_queue_tail = NULL;
static thread_t *kernel_idle_task;
static spinlock_t sched_lock = SPINLOCK_RELEASED;

void idle_task(void *arg) {
  while (1) {
    thread_t *next = sched_pick_next();
    dispatch_switch_from_kernel(next);
  }
}

void sched_init(void) {
  kernel_idle_task = thread_create_kernel(NULL, (uintptr_t)idle_task);

  ready_queue_head = NULL;
  ready_queue_tail = NULL;
}

void sched_enqueue(thread_t *t) {
  if (!t)
    return;

  spinlock_acquire(&sched_lock);

  t->state = THREAD_READY;
  t->next = NULL;

  // Add to back of queue (FCFS)
  if (!ready_queue_tail) {
    ready_queue_head = t;
    ready_queue_tail = t;
  } else {
    ready_queue_tail->next = t;
    ready_queue_tail = t;
  }

  spinlock_release(&sched_lock);
}

void sched_dequeue(thread_t *t) {
  if (!t)
    return;

  spinlock_acquire(&sched_lock);

  // Remove from queue
  if (ready_queue_head == t) {
    ready_queue_head = t->next;
    if (ready_queue_tail == t) {
      ready_queue_tail = NULL;
    }
  } else {
    thread_t *curr = ready_queue_head;
    while (curr && curr->next != t) {
      curr = curr->next;
    }
    if (curr) {
      curr->next = t->next;
      if (ready_queue_tail == t) {
        ready_queue_tail = curr;
      }
    }
  }

  t->next = NULL;
  t->state = THREAD_DEAD;

  spinlock_release(&sched_lock);
}

thread_t *sched_pick_next(void) {
  spinlock_acquire(&sched_lock);

  thread_t *next = ready_queue_head;

  if (next) {
    // 1. REMOVE the thread from the ready queue
    ready_queue_head = next->next;

    // 2. If the queue is now empty, update the tail
    if (ready_queue_head == NULL) {
      ready_queue_tail = NULL;
    }

    // 3. Clean the next pointer so the thread is isolated
    next->next = NULL;
  } else {
    // 4. If queue is empty, return idle task
    // Note: We do NOT modify ready_queue_head here
    next = kernel_idle_task;
  }

  spinlock_release(&sched_lock);
  return next;
}

void sched_tick(void *context) {}
