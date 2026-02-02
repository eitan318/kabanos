#include "sched/sched.h"
#include "hal.h"
#include "memory_management/kmalloc.h"
#include <stdio.h>

static thread_t *ready_list = NULL;
thread_t *g_current_thread = NULL;
static thread_t kernel_idle_task;

void sched_remove(thread_t *t) {
  if (!t || !ready_list)
    return;

  // Case 1: The thread to remove is at the head of the list
  if (ready_list == t) {
    ready_list = t->next;
  } else {
    // Case 2: Walk the list to find the thread
    thread_t *curr = ready_list;
    while (curr->next && curr->next != t) {
      curr = curr->next;
    }

    // If found, unlink it
    if (curr->next == t) {
      curr->next = t->next;
    }
  }

  t->state = THREAD_DEAD;
  t->next = NULL;

  // If we just removed the thread that was currently running,
  // we must immediately trigger a reschedule.
  if (g_current_thread == t) {
    // In some kernels, you'd call yield() or force a timer interrupt here
    g_current_thread = NULL;
  }
}

void sched_add(thread_t *t) {
  if (!t)
    return;
  t->state = THREAD_READY;

  // Simple push to front of list
  t->next = ready_list;
  ready_list = t;
}

static thread_t *sched_next(void) {
  if (!ready_list)
    return &kernel_idle_task;

  // 1. If there's only one thread, just return it
  if (ready_list->next == NULL) {
    return (ready_list->state == THREAD_READY) ? ready_list : &kernel_idle_task;
  }

  // 2. Rotate the list: Take the current head and move it to the tail
  thread_t *prev_head = ready_list;
  ready_list = ready_list->next; // New head
  prev_head->next = NULL;

  // Find the current tail to attach the old head
  thread_t *tail = ready_list;
  while (tail->next != NULL) {
    tail = tail->next;
  }
  tail->next = prev_head;

  // 3. Return the new head (if it's ready to run)
  if (ready_list->state == THREAD_READY ||
      ready_list->state == THREAD_RUNNING) {
    return ready_list;
  }

  return &kernel_idle_task;
}

void sched_tick(void *context) {
  // 1. Save current state
  if (g_current_thread && g_current_thread != &kernel_idle_task) {
    hal_thread_save(g_current_thread->arch, context);
    if (g_current_thread->state == THREAD_RUNNING) {
      g_current_thread->state = THREAD_READY;
    }
  }

  // 2. Pick next
  thread_t *next = sched_next();

  // 3. Prepare for switch
  g_current_thread = next;
  next->state = THREAD_RUNNING;

  // Update TSS/MSR so sysenter/interrupts land on the correct kernel stack
  hal_update_kernel_stack(0, next->kstack_top);

  // 4. Perform the switch
  hal_thread_switch(next);
}

void sched_init(void) {
  // Initialize a dummy idle task so the scheduler never has NULL
  kernel_idle_task.tid = 0;
  kernel_idle_task.state = THREAD_READY;
  kernel_idle_task.kstack_top = kmalloc(4096) + 4096;
  g_current_thread = &kernel_idle_task;
}
