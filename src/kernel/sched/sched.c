#include "sched/sched.h"
#include "hal.h"
#include "sched/thread.h"
#include "memory_management/kmalloc.h"
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

static thread_t *sched_next(void) {
	if (tasks_list == NULL) {
		return NULL;
	}

	// Move to next node in circular list
	current_node = current_node->next;
	return current_node->thread;
}

extern void __attribute__((naked)) switch_to(thread_t *p);

void sched_tick(struct regs *r) {
  if (current == NULL) {
    current = sched_next();
  } else {
    current->kernel_esp = (void *)r;
  }

  thread_t *next = sched_next();
  if (!next) {
    next = &kernel_task;
  }
  current = next;

  uint32_t cpu_id = 0;
  hal_set_kernel_stack(cpu_id, next->kstack_top);

  switch_to(next);
}

void sched_init(void) { current = NULL; }
