#include "sched/sched.h"
#include "sched/spinlock.h"
#include "hal.h"
#include "memory_management/kmalloc.h"
#include <stdio.h>

static thread_t *normal_list = NULL;
static thread_t *above_normal_list = NULL;
static thread_t *high_list = NULL;
static thread_t *realtime_list = NULL;
thread_t *g_current_thread = NULL;
static thread_t kernel_idle_task;
static spinlock_t sched_lock = SPINLOCK_RELEASED;

static thread_t *list_remove(thread_t *head, thread_t *t) {
  if (head == t) {
    return t->next;
  }

  thread_t *curr = head;
  while (curr->next != NULL && curr->next != t) {
    curr = curr->next;
  }

  if (curr->next == t) {
    curr->next = t->next;
  }

  return head;
}

static thread_t **list_for_priority(enum thread_priority p) {
  switch (p) {
	  case THREAD_REALTIME:     
		return &realtime_list;
	  case THREAD_HIGH:         
		return &high_list;
	  case THREAD_ABOVE_NORMAL: 
		return &above_normal_list;
	  default:                  
		return &normal_list;
  }
}

static long quantom_time_get(enum thread_priority p) { 
	switch (p) {
	  case THREAD_REALTIME:     
		return QUANTOM_TIME_REALTIME;
	  case THREAD_HIGH:         
		return QUANTOM_TIME_HIGH;
	  case THREAD_ABOVE_NORMAL: 
		return QUANTOM_TIME_ABOVE_NORMAL;
	  default:                  
		return QUANTOM_TIME_NORMAL;
  }	
}

void sched_remove(thread_t *t) {
  if (!t) {
    return;
  }
  
  spinlock_acquire(&sched_lock);

  thread_t **list = list_for_priority(t->priority);
  *list = list_remove(*list, t);

  t->state = THREAD_DEAD;
  t->next  = NULL;

  if (g_current_thread == t) {
    g_current_thread = NULL;
  }
  
  spinlock_release(&sched_lock);
}

void sched_add(thread_t *t) {
  if (!t) {
    return;
  }
  
  spinlock_acquire(&sched_lock);
  
  t->state = THREAD_READY;

  thread_t **list = list_for_priority(t->priority);

  /* Push to front of the correct priority list */
  t->next = *list;
  *list = t;
  
  spinlock_release(&sched_lock);
}

static thread_t *roundrobin_pick(thread_t **list_head) {
  thread_t *list = *list_head;

  /* Empty list - nothing to pick. */
  if (!list) {
    return NULL;
  }

  /* Single thread - no rotation needed. */
  if (!list->next) {
    return (list->state == THREAD_READY || list->state == THREAD_RUNNING) ? list : NULL;
  }

  /* Rotate: move current head to tail. */
  thread_t *old_head = list;
  *list_head = list->next;  
  old_head->next = NULL;

  /* Walk to tail and append old head. */
  thread_t *tail = *list_head;
  while (tail->next != NULL) {
    tail = tail->next;
  }
  tail->next = old_head;

  /* Return new head only if it is schedulable. */
  thread_t *candidate = *list_head;
  if (candidate->state == THREAD_READY || candidate->state == THREAD_RUNNING) {
    return candidate;
  }

  return NULL;
}

static thread_t *normal_list_pick(void) {
	return roundrobin_pick(&normal_list);
}

static thread_t *above_normal_list_pick(void) {
	return roundrobin_pick(&above_normal_list);
}

static thread_t *high_list_pick(void) {
	return roundrobin_pick(&high_list);
}

static thread_t *realtime_list_pick(void) {
	return roundrobin_pick(&realtime_list);
}

static thread_t *sched_next(void) {
  thread_t *t;

  // Walk priority levels from highest to lowest.
  if ((t = realtime_list_pick()) != NULL) {
    return t;
  }
  if ((t = high_list_pick()) != NULL) {
    return t;
  }
  if ((t = above_normal_list_pick()) != NULL) {
    return t;
  }
  if ((t = normal_list_pick()) != NULL) {
    return t;
  }

  return &kernel_idle_task;
}

void sched_tick(void *context) {
  spinlock_acquire(&sched_lock);
  
  // 1. Save current state
  if (g_current_thread && g_current_thread != &kernel_idle_task) {
    if (g_current_thread->rt_ticks < quantom_time_get(g_current_thread->priority)) {
      g_current_thread->rt_ticks++;
      spinlock_release(&sched_lock);
      return;
    } else {
	  g_current_thread->rt_ticks = 0;
	}
	  
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
  
  spinlock_release(&sched_lock);
  
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
