#pragma once
#include "proc/proc.h"
#include <stdint.h>

enum thread_mode { THREAD_MODE_KERNEL, THREAD_MODE_USER };

typedef struct thread {
  arch_thread_t *arch;
  uint32_t tid;
  process_t *process; // Parent process (contains CR3)

  uint32_t rt_ticks;
  uint32_t time_slice_remaining;

  enum thread_state {
    THREAD_NEW,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_DEAD,
    THREAD_BLOCKED,
  } state;

  enum thread_priority {
    PRIORITY_VERY_HIGH = 0,
    PRIORITY_HIGH = 1,
    PRIORITY_MEDIUM = 2,
    PRIORITY_LOW = 3,
    NUM_PRIORITIES = 4
  } priority;

  enum thread_mode mode;

  /* Stack tracking (for cleanup only) */
  void *kstack_top; // Top of kernel stack (for deallocation)

  /* Scheduler linkage */
  struct thread *next;
} thread_t;

thread_t *thread_create_user(process_t *proc, uintptr_t entry,
                             uintptr_t user_stack, enum thread_priority p);

thread_t *thread_create_kernel(process_t *proc, uintptr_t entry);
