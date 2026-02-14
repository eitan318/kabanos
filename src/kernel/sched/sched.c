#include "sched.h"
#include "dispatcher.h"
#include "memory_management/kmalloc.h"
#include "sched/thread.h"
#include "stdio.h"

// Time quantums (in milliseconds)
static const int time_quantums[NUM_PRIORITIES] = {[PRIORITY_VERY_HIGH] = 40,
                                                  [PRIORITY_HIGH] = 80,
                                                  [PRIORITY_MEDIUM] = 120,
                                                  [PRIORITY_LOW] = 200};

// Queue heads and tails
static thread_t *ready_queue_heads[NUM_PRIORITIES] = {NULL};
static thread_t *ready_queue_tails[NUM_PRIORITIES] = {NULL};

static thread_t *kernel_idle_task = NULL;
static spinlock_t sched_lock = SPINLOCK_RELEASED;

static uint32_t tick = 0;

void idle_task(void *arg) {
  while (1) {
    __asm__ volatile("sti; hlt");
  }
}

void sched_init(void) {
  kernel_idle_task = thread_create_kernel(NULL, (uintptr_t)idle_task);
  kernel_idle_task->tid = 0;
  kernel_idle_task->priority = PRIORITY_LOW; // Doesn't matter, never enqueued

  for (int i = 0; i < NUM_PRIORITIES; i++) {
    ready_queue_heads[i] = NULL;
    ready_queue_tails[i] = NULL;
  }
}

void print_thread_struct(thread_t *t) {
  if (!t)
    return;
  debugf_and_printf("\tTID %d {priority: %d ticks yet: %d}", t->tid,
                    t->priority, t->rt_ticks);
}
void print_sched_struct() {

  debugf_and_printf("\nCurrent: ");
  print_thread_struct(dispatch_get_current());
  debugf_and_printf("\n");
  for (int i = 0; i < NUM_PRIORITIES; i++) {
    debugf_and_printf("Priority %d: \n", i);

    for (thread_t *curr = ready_queue_heads[i]; curr != NULL;
         curr = curr->next) {
      print_thread_struct(curr);
      debugf_and_printf("\n");
    }
  }
  debugf_and_printf("\n");
}

void sched_enqueue(thread_t *t) {
  if (!t || t->tid == 0) // Don't enqueue idle
    return;

  spinlock_acquire(&sched_lock);

  int priority = t->priority;
  if (priority < 0 || priority >= NUM_PRIORITIES) {
    priority = PRIORITY_MEDIUM; // Default
  }

  t->state = THREAD_READY;
  t->next = NULL;

  // Add to back of appropriate priority queue
  if (!ready_queue_tails[priority]) {
    ready_queue_heads[priority] = t;
    ready_queue_tails[priority] = t;
  } else {
    ready_queue_tails[priority]->next = t;
    ready_queue_tails[priority] = t;
  }

  // Reset time quantum for this priority
  t->time_slice_remaining = time_quantums[priority];

  spinlock_release(&sched_lock);
}

thread_t *sched_pick_next(void) {
  spinlock_acquire(&sched_lock);

  // Check from highest to lowest priority
  for (int priority = 0; priority < NUM_PRIORITIES; priority++) {
    if (ready_queue_heads[priority]) {
      thread_t *next = ready_queue_heads[priority];

      // Remove from queue
      ready_queue_heads[priority] = next->next;
      if (!ready_queue_heads[priority]) {
        ready_queue_tails[priority] = NULL;
      }
      next->next = NULL;

      spinlock_release(&sched_lock);
      return next;
    }
  }

  // No ready threads - return idle
  spinlock_release(&sched_lock);
  return kernel_idle_task;
}

void sched_tick(void *context) {

  thread_t *current = dispatch_get_current();
  if (!current) {
    return; // Safety check
  }

  // Don't preempt idle or blocked threads
  if (current->tid == 0 || current->state != THREAD_RUNNING) {
    return;
  }

  if ((++tick) % 10 == 0) {
    print_sched_struct();
  }

  // Decrement time slice
  current->rt_ticks++;
  current->time_slice_remaining--;

  // Check if time slice expired
  if (current->time_slice_remaining <= 0) {
    // Time slice expired - re-enqueue and pick next
    sched_enqueue(current);

    thread_t *next = sched_pick_next();
    if (next && next != current) {
      dispatch_switch_from_interrupt(context, next);
    }
  }
}

void sched_dequeue(thread_t *t) {
  if (!t || t->tid == 0)
    return;

  spinlock_acquire(&sched_lock);

  int priority = t->priority;
  if (priority < 0 || priority >= NUM_PRIORITIES) {
    spinlock_release(&sched_lock);
    return;
  }

  // Remove from queue
  if (ready_queue_heads[priority] == t) {
    ready_queue_heads[priority] = t->next;
    if (ready_queue_tails[priority] == t) {
      ready_queue_tails[priority] = NULL;
    }
  } else {
    thread_t *curr = ready_queue_heads[priority];
    while (curr && curr->next != t) {
      curr = curr->next;
    }
    if (curr) {
      curr->next = t->next;
      if (ready_queue_tails[priority] == t) {
        ready_queue_tails[priority] = curr;
      }
    }
  }

  t->next = NULL;
  t->state = THREAD_DEAD;

  spinlock_release(&sched_lock);
}
