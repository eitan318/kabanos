#pragma once
#include "arch/types.h"
#include "stdint.h"

typedef struct process process_t;

enum thread_mode { THREAD_MODE_KERNEL, THREAD_MODE_USER };

typedef struct thread {
  arch_thread_t *arch;
  uint32_t tid;
  process_t *process;

  // sched info
  uint32_t rt_ticks;
  uint32_t curr_time_quantum;
  uint32_t curr_time_quantum_ticks_passed;
  uint32_t burst_ticks_estimate;

  uint32_t last_enqueue_tick;

  // sleep info
  uint32_t wakeup_time;

  enum thread_state {
    THREAD_STATE_NEW,
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_DEAD,
    THREAD_STATE_BLOCKED,
  } state;

  enum thread_priority {
    THREAD_PRIORITY_VERY_HIGH = 0,
    THREAD_PRIORITY_HIGH = 1,
    THREAD_PRIORITY_MEDIUM = 2,
    THREAD_PRIORITY_LOW = 3,
    THREAD_NUM_PRIORITIES = 4
  } priority;

  enum thread_mode mode;

  void *kstack_top; // Top of kernel stack (for deallocation)

  // Scheduler linkage
  struct thread *next;
  // Sleeping linkage
  struct thread *next_sleep;
} thread_t;

thread_t *thread_create_user(process_t *proc, uintptr_t entry,
                             uintptr_t user_stack, enum thread_priority p);
thread_t *thread_create_kernel(process_t *proc, uintptr_t entry);
thread_t *thread_clone(thread_t *parent, process_t *child_proc);
