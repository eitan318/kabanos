#pragma once
#include <stdbool.h>
#include <stdint.h>

// Forward declaration
typedef struct Pcb Pcb;

// Process states
typedef enum {
  PROCESS_STATE_NEW,
  PROCESS_STATE_READY,
  PROCESS_STATE_RUNNING,
  PROCESS_STATE_WAITING,
  PROCESS_STATE_TERMINATED
} ProcessState;

// Process priorities
typedef enum {
  PROCESS_PRIORITY_LOW,
  PROCESS_PRIORITY_NORMAL,
  PROCESS_PRIORITY_HIGH
} ProcessPriority;

// CPU context structure (i686 specific)
typedef struct {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
  uint32_t esi;
  uint32_t edi;
  uint32_t ebp;
  uint32_t esp;
  uint32_t eip;
  uint32_t eflags;
  uint32_t cr3;
} CpuContext;

// Process Control Block structure
struct Pcb {
  uint32_t pid;
  char name[32];
  ProcessState state;
  ProcessPriority priority;

  CpuContext cpu_context;

  uint32_t stack_base;
  uint32_t stack_size;
  uint32_t stack_top;

  uint32_t heap_start;
  uint32_t heap_end;
  uint32_t brk;

  uint32_t *page_directory;

  uint32_t time_slice;
  uint32_t time_used;

  void *wait_object;
  uint32_t wait_timeout;

  uint32_t parent_pid;
  int exit_code;
};

// PCB lifecycle management
Pcb *pcb_create(uint32_t pid, const char *name, ProcessPriority priority);
void pcb_destroy(Pcb *pcb);

// Context management
void pcb_context_init(Pcb *pcb, uint32_t entry_point, uint32_t stack_top);

// Setters
void pcb_state_set(Pcb *pcb, ProcessState state);
void pcb_priority_set(Pcb *pcb, ProcessPriority priority);

// Debug helpers
const char *pcb_state_string_get(ProcessState state);
const char *pcb_priority_string_get(ProcessPriority priority);
