#include "sched/sched.h"
#include "hal.h"
#include "isr.h"
#include "memory_management/kmalloc.h"
#include "sched/spinlock.h"
#include "sched/thread.h"
#include <stddef.h>
#include <stdint.h>

extern vmspace_t *g_kernel_vmspace; // global

typedef struct task_node {
  thread_t *thread;
  struct task_node *next;
} task_node_t;

static task_node_t *tasks_list = NULL;
static task_node_t *current_node = NULL;

static thread_t kernel_task;
thread_t *current = NULL;
static spinlock_t ready_lock = SPINLOCK_RELEASED;

void sched_add(thread_t *t) {
  task_node_t *new_node = (task_node_t *)kmalloc(sizeof(task_node_t));
  new_node->thread = t;

  if (tasks_list == NULL) {
    // First node - points to itself (circular)
    new_node->next = new_node;
    tasks_list = new_node;
    current_node = new_node;
  } else {
    // Insert at the end and maintain circular structure
    task_node_t *tail = tasks_list;
    while (tail->next != tasks_list) {
      tail = tail->next;
    }
    new_node->next = tasks_list;
    tail->next = new_node;
  }
}

void sched_remove(thread_t *t) {
  if (!tasks_list || !t) {
    return;
  }

  task_node_t *prev = tasks_list;
  task_node_t *node = tasks_list;

  /* Find node to remove */
  do {
    if (node->thread == t) {
      break;
    }
    prev = node;
    node = node->next;
  } while (node != tasks_list);

  /* Not found */
  if (node->thread != t) {
    return;
  }

  /* Single-node list */
  if (node->next == node) {
    tasks_list = NULL;
    current_node = NULL;
    if (current == t) {
      current = NULL;
    }
    kfree(node);
    return;
  }

  /* Fix head if needed */
  if (node == tasks_list) {
    tasks_list = node->next;
  }

  /* Fix current_node if needed */
  if (node == current_node) {
    current_node = node->next;
  }

  /* Fix current running thread */
  if (current == t) {
    current = NULL; // scheduler will pick next on tick
  }

  /* Unlink */
  prev->next = node->next;

  kfree(node);
}

static thread_t *sched_next(void) {
  if (!tasks_list || !current_node) {
    return NULL;
  }

  task_node_t *start = current_node;

  do {
    current_node = current_node->next;

    if (current_node->thread->state == THREAD_REALTIME) {
      return current_node->thread;
    }

  } while (current_node != start);

  do {
    current_node = current_node->next;

    if (current_node->thread->state == THREAD_READY) {
      return current_node->thread;
    }

  } while (current_node != start);

  return NULL;
}

void debug_ticker(struct arch_regs *r) { sched_tick(r); }

void sched_tick(void *context) {
  if (current != NULL) {
    hal_thread_save(current->arch, context);
  }

  // Let THREAD_REALTIME run twice
  if (current && current->state == THREAD_REALTIME && current->rt_ticks == 0) {
    current->rt_ticks = 1;
    return;
  }

  // Reset after second tick
  if (current && current->state == THREAD_REALTIME) {
    current->rt_ticks = 0;
  }

  thread_t *next = sched_next();
  if (!next) {
    next = &kernel_task;
  }

  if (current->state == THREAD_RUN) {
    current->state = THREAD_READY;
  }
  current = next;

  int cpu_id = 0;
  hal_set_kernel_stack(cpu_id, next->kstack_top);
  next->state = THREAD_RUN;
  hal_thread_switch(next);
}

void sched_init(void) {
  isr_handler_register(0x45, debug_ticker);
  current = NULL;
}
