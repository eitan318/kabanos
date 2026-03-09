#include "sched/sched.h"
#include "hal.h"
#include "klib/stdio.h"
#include "mm/kmalloc.h"
#include "modules.h"
#include "sched/dispatcher.h"
#include "sched/sleep.h"
#include "sched/thread.h"
#include "sched/timer.h"
#include "spinlock.h"

#define AGING_THRESHOLD_MS 500

// Time quantums (in milliseconds)
static const int time_quantums[THREAD_NUM_PRIORITIES] = {
    [THREAD_PRIORITY_VERY_HIGH] = 40,
    [THREAD_PRIORITY_HIGH] = 80,
    [THREAD_PRIORITY_MEDIUM] = 120,
    [THREAD_PRIORITY_LOW] = 200};

// Queue heads and tails
static thread_t *ready_queue_heads[THREAD_NUM_PRIORITIES] = {NULL};
static thread_t *ready_queue_tails[THREAD_NUM_PRIORITIES] = {NULL};

static thread_t *kernel_idle_task = NULL;
static spinlock_t sched_lock = SPINLOCK_RELEASED;

void idle_task(void *arg) {
  while (1) {
    hal_interrupts_enable();
    hal_halt();
  }
}

int sched_init(module_t *self) {
  kernel_idle_task = thread_create_kernel(NULL, (uintptr_t)idle_task);
  kernel_idle_task->tid = 0;
  kernel_idle_task->priority =
      THREAD_PRIORITY_LOW;                 // Doesn't matter, never enqueued
  kernel_idle_task->curr_time_quantum = 0; // IDLE immediatly swapped

  for (int i = 0; i < THREAD_NUM_PRIORITIES; i++) {
    ready_queue_heads[i] = NULL;
    ready_queue_tails[i] = NULL;
  }

  return 0;
}

void print_thread_struct(thread_t *t) {
  if (!t)
    return;
  kprintf("\tTID %d {priority: %d ticks yet: %d}", t->tid, t->priority,
          t->rt_ticks);
}

void print_sched_struct() {
  kprintf("\nCurrent: ");
  print_thread_struct(dispatch_get_current());
  kprintf("\n");
  for (int i = 0; i < THREAD_NUM_PRIORITIES; i++) {
    kprintf("Priority %d: \n", i);

    for (thread_t *curr = ready_queue_heads[i]; curr != NULL;
         curr = curr->next) {
      print_thread_struct(curr);
      kprintf("\n");
    }
  }
  kprintf("\n");
}

void sched_enqueue(thread_t *t) {
  if (!t || t->tid == 0) // Don't enqueue idle
    return;

  spinlock_acquire(&sched_lock);

  int priority = t->priority;
  if (priority < 0 || priority >= THREAD_NUM_PRIORITIES) {
    priority = THREAD_PRIORITY_MEDIUM; // Default
  }

  t->state = THREAD_STATE_READY;
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
  t->curr_time_quantum = time_quantums[priority];
  t->burst_ticks_estimate = t->curr_time_quantum;
  t->last_enqueue_tick = timer_tick_get();

  spinlock_release(&sched_lock);
}

void sched_prepare_for_cpu_burst(thread_t *t) {
  // Formula for history based prediction: T<n+1> = t<n> * a + (1-a) * T<n>
  const double a = 0.5;
  int next_burst_ticks_estimate = (t->curr_time_quantum_ticks_passed * a) +
                                  ((1 - a) * t->burst_ticks_estimate);

  // Classify priority based on burst length
  // Shorter bursts = higher priority (more interactive/I/O bound)
  // Longer bursts = lower priority (more CPU bound)
  t->priority = THREAD_PRIORITY_LOW; // Default to lowest

  for (int i = 0; i < THREAD_NUM_PRIORITIES; i++) {
    if (next_burst_ticks_estimate <= time_quantums[i]) {
      t->priority = i;
      break;
    }
  }

  t->burst_ticks_estimate = next_burst_ticks_estimate;
  t->curr_time_quantum = time_quantums[t->priority];
  t->curr_time_quantum_ticks_passed = 0;
}

void sched_apply_aging(void) {
  // We start from 1 because Priority 0 is already the highest
  for (int i = 1; i < THREAD_NUM_PRIORITIES; i++) {
    thread_t *curr = ready_queue_heads[i];
    thread_t *prev = NULL;

    while (curr != NULL) {
      uint32_t wait_time = timer_tick_get() - curr->last_enqueue_tick;

      if (wait_time >= (AGING_THRESHOLD_MS / TIMER_TICK_MS)) {
        thread_t *to_promote = curr;

        // 1. Remove from current queue
        if (prev) {
          prev->next = curr->next;
        } else {
          ready_queue_heads[i] = curr->next;
        }

        if (ready_queue_tails[i] == to_promote) {
          ready_queue_tails[i] = prev;
        }

        curr = curr->next; // Move iterator to next thread

        // 2. Promote and Enqueue in higher priority (i - 1)
        to_promote->priority = i - 1;
        to_promote->next = NULL;
        to_promote->last_enqueue_tick = timer_tick_get(); // Reset wait clock

        if (!ready_queue_tails[i - 1]) {
          ready_queue_heads[i - 1] = to_promote;
          ready_queue_tails[i - 1] = to_promote;
        } else {
          ready_queue_tails[i - 1]->next = to_promote;
          ready_queue_tails[i - 1] = to_promote;
        }
      } else {
        prev = curr;
        curr = curr->next;
      }
    }
  }
}

thread_t *sched_pick_next(void) {
  spinlock_acquire(&sched_lock);

  // Check from highest to lowest priority
  for (int priority = 0; priority < THREAD_NUM_PRIORITIES; priority++) {
    if (ready_queue_heads[priority]) {
      thread_t *next = ready_queue_heads[priority];

      // Remove from queue
      ready_queue_heads[priority] = next->next;
      if (!ready_queue_heads[priority]) {
        ready_queue_tails[priority] = NULL;
      }
      next->next = NULL;
      sched_prepare_for_cpu_burst(next);

      spinlock_release(&sched_lock);
      return next;
    }
  }

  // No ready threads - return idle
  spinlock_release(&sched_lock);
  return kernel_idle_task;
}

void sched_on_timer_tick(void *context) {
  wake_up_sleeping(timer_tick_get());

  if (timer_tick_get() % 100 == 0) {
    spinlock_acquire(&sched_lock);
    sched_apply_aging();
    spinlock_release(&sched_lock);
  }

  thread_t *current = dispatch_get_current();
  if (!current) {
    return; // Safety check
  }

  // Don't preempt idle or blocked threads
  if (current->state != THREAD_STATE_RUNNING) {
    return;
  }

  current->rt_ticks++;
  current->curr_time_quantum_ticks_passed++;

  int remain_time =
      current->curr_time_quantum - current->curr_time_quantum_ticks_passed;

  // Check if time slice expired
  if (remain_time <= 0) {
    sched_enqueue(current);
    sched_switch_next();
  }
}

void sched_dequeue(thread_t *t) {
  if (!t || t->tid == 0)
    return;

  spinlock_acquire(&sched_lock);

  int priority = t->priority;
  if (priority < 0 || priority >= THREAD_NUM_PRIORITIES) {
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
  t->state = THREAD_STATE_DEAD;

  spinlock_release(&sched_lock);
}

void sched_switch_next() {
  thread_t *next = sched_pick_next();
  dispatch_switch_to(next);
}

static const char *sched_deps[] = {"dispatcher", "timer", NULL};

ITER_MODULE(sched) = {
    .name = "sched",
    .required_modules_names = sched_deps,
    .init = &sched_init,
    .fini = NULL,
};
;
