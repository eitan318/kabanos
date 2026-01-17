#pragma once
#include "memory_management/vmspace.h"
#include <stdint.h>

typedef struct process {
  uint32_t pid;
  vmspace_t *vmspace;
  struct thread *main_thread;
} process_t;

process_t *process_create(void);
void process_destroy(process_t *proc);
