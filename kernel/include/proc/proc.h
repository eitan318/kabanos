/**
 * @file proc.h
 * @brief Process management structures and system call prototypes.
 */

#pragma once
#include "mm/vmspace.h"
#include "stdint.h"

typedef uint32_t pid_t;

/** @brief Table of open file descriptors/devices for a process. */
typedef struct {
  struct device *devices[16]; /**< Fixed-size array of device pointers. */
} fd_table_t;

/** @brief Current lifecycle state of a process. */
typedef enum process_state {
  PROCESS_ALIVE,  /**< Process is currently active or runnable. */
  PROCESS_ZOMBIE, /**< Process has exited but has not been reaped by parent. */
} process_state_t;

/** @brief The process control block (PCB). */
typedef struct process {
  uint32_t pid;               /**< Unique Process ID. */
  vmspace_t *vmspace;         /**< Virtual memory space and page tables. */
  fd_table_t fd_table;        /**< Open file descriptors. */
  struct thread *main_thread; /**< The primary thread of execution. */

  process_state_t state; /**< Current process state. */
  int exit_code;         /**< Exit status to be returned to parent. */

  struct process *parent; /**< Pointer to the parent process. */
  bool is_waiting;        /**< True if the process is blocked in waitpid. */

  struct process *first_child;  /**< Head of the linked list of children. */
  struct process *next_sibling; /**< Next sibling in the parent's child list. */
} process_t;

/**
 * @brief Allocates and initializes a new process structure.
 * @return Pointer to the new process_t, or NULL on failure.
 */
process_t *process_create(void);

/**
 * @brief Frees all resources associated with a process.
 * @param proc Pointer to the process to destroy.
 */
void process_destroy(process_t *proc);
