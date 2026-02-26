#pragma once
#include "mm/vmspace.h"
#include <stdint.h>

typedef struct {
  struct device *devices[16]; // This process can open 16 things
} fd_table_t;

typedef enum process_state {
  PROCESS_ALIVE,
  PROCESS_ZOMBIE,
} process_state_t;

typedef struct process {
  uint32_t pid;
  vmspace_t *vmspace;
  fd_table_t fd_table;
  struct thread *main_thread;

  process_state_t state;
  int exit_code;

  struct process *parent;
  bool is_waiting;

} process_t;

process_t *process_create(void);
void process_destroy(process_t *proc);
