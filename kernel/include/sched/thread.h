/**
 * @file thread.h
 * @brief Thread control block and thread creation.
 */
#pragma once
#include "arch/types.h"
#include "klib/stddef.h"
#include "klib/stdint.h"

typedef struct process process_t;

enum thread_mode { THREAD_MODE_KERNEL, THREAD_MODE_USER };

/** @brief A schedulable thread of execution. */
typedef struct thread {
  arch_thread_t *arch; /**< Arch-specific context (saved stack pointer). */
  uint32_t tid;
  process_t *process; /**< Owning process. */

  /* Scheduler accounting */
  uint32_t rt_ticks;                       /**< Total ticks spent running. */
  uint32_t curr_time_quantum;              /**< Length of the current slice. */
  uint32_t curr_time_quantum_ticks_passed; /**< Ticks used of the slice. */
  uint32_t burst_ticks_estimate; /**< CPU burst estimate for scheduling. */

  uint32_t last_enqueue_tick; /**< When the thread was last made ready. */

  /* Sleep info */
  uint32_t wakeup_time; /**< Tick at which a sleeping thread wakes. */

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

  enum thread_mode mode; /**< Kernel or user thread. */

  void *kstack_top; /**< Top of the kernel stack (for deallocation). */

  struct thread *next;       /**< Scheduler queue linkage. */
  struct thread *next_sleep; /**< Sleeper list linkage. */
} thread_t;

/**
 * @brief Creates a user-mode thread that starts at @p entry with its
 *        stack at @p user_stack.
 */
thread_t *thread_create_user(process_t *proc, uintptr_t entry,
                             uintptr_t user_stack, enum thread_priority p);

/** @brief Creates a kernel-mode thread starting at @p entry. */
thread_t *thread_create_kernel(process_t *proc, uintptr_t entry);

/** @brief Duplicates @p parent for @p child_proc (fork). */
thread_t *thread_clone(thread_t *parent, process_t *child_proc);
