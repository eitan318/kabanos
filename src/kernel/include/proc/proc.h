#pragma once
#include "klib/stdint.h"
#include "mm/vmspace.h"

typedef uint32_t pid_t;

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

  struct process *first_child;
  struct process *next_sibling;

} process_t;

process_t *process_create(void);
void process_destroy(process_t *proc);

long sys_fork();
void sys_exit(int status);
pid_t sys_waitpid(pid_t target_pid, int *wstatus, int options);
long sys_getpid();
