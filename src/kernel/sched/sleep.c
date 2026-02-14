#include "sleep.h"
#include "sched/sched.h"
#include <stddef.h>
#include <stdio.h>

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
