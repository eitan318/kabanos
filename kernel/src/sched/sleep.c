#include "sched/sleep.h"
#include "delay.h"
#include "klib/stddef.h"
#include "klib/time.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/timer.h"
#include "syscall_errno.h"

thread_t *sleep_queue_head;

void wake_up_sleeping(uint32_t g_curr_tick) {
  while (sleep_queue_head && sleep_queue_head->wakeup_time <= g_curr_tick) {
    thread_t *thread_to_wake = sleep_queue_head;

    // Advance the head using the dedicated sleep pointer
    sleep_queue_head = thread_to_wake->next_sleep;
    thread_to_wake->next_sleep = NULL;

    // Return to the scheduler
    sched_enqueue(thread_to_wake);
  }
}

void enqueue_sleeper(thread_t *t, uint32_t wake_up_time) {
  t->wakeup_time = wake_up_time;
  t->next_sleep = NULL;

  // Case 1: List is empty or new thread wakes up sooner than current head
  if (!sleep_queue_head || t->wakeup_time < sleep_queue_head->wakeup_time) {
    t->next_sleep = sleep_queue_head;
    sleep_queue_head = t;
    return;
  }

  // Case 2: Traverse to find the correct spot (need to track 'prev')
  thread_t *curr = sleep_queue_head;
  while (curr->next_sleep && curr->next_sleep->wakeup_time <= t->wakeup_time) {
    curr = curr->next_sleep;
  }

  // Insert t between curr and curr->next_sleep
  t->next_sleep = curr->next_sleep;
  curr->next_sleep = t;
}

void sys_sleep(uint32_t seconds) {
  thread_t *current = dispatch_get_current();
  uint32_t curr_tick = timer_tick_get();
  enqueue_sleeper(current, curr_tick + ((seconds * 1000) / TIMER_TICK_MS));
  sched_switch_next();
}

int sys_nanosleep(const timespec_t *req, timespec_t *rem) {
  if (!req)
    return -EFAULT;

  uint64_t total_ns = (uint64_t)req->tv_sec * 1000000000ULL + req->tv_nsec;
  ticks_t delta_ticks = (total_ns * TIMER_HZ) / 1000000000ULL;

  // If the delta is smaller than a tick - busy wait
  if (delta_ticks == 0 && total_ns > 0) {
    extern uint64_t g_cpu_loops_per_ns;
    ndelay(total_ns, g_cpu_loops_per_ns);
  }

  thread_t *current = dispatch_get_current();
  current->state = THREAD_STATE_BLOCKED;

  ticks_t start_tick = timer_tick_get();
  enqueue_sleeper(current, start_tick + delta_ticks);

  sched_switch_next();

  // TODO:Handling 'rem' (Remaining time)
  // If the sleep was interrupted by a signal, we should calculate
  // how much time is left and write it to *rem.
  return 0;
}
