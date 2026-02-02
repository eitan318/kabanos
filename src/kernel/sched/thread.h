#pragma once
#include "hal.h"
#include "proc/proc.h"
#include <stdint.h>

typedef struct thread {
  uint32_t tid;
  process_t *process; // Parent process (contains CR3)

  void *kernel_esp;
  
  uint8_t rt_ticks;
  enum thread_state { THREAD_NORMAL, THREAD_ABOVE_NORMAL, THREAD_HIGH, THREAD_REALTIME, THREAD_DEAD } state;
  enum thread_mode mode;

  /* Stack tracking (for cleanup only) */
  void *kstack_top; // Top of kernel stack (for deallocation)

  /* Scheduler linkage */
  struct thread *next;
} thread_t;

thread_t *thread_create_user(process_t *proc, uintptr_t entry,
                             uintptr_t user_stack);

thread_t *thread_create_kernel(process_t *proc, uintptr_t entry);
