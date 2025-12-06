#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Process states
typedef enum {
  PROCESS_STATE_NEW,
  PROCESS_STATE_READY,
  PROCESS_STATE_RUNNING,
  PROCESS_STATE_WAITING,
  PROCESS_STATE_TERMINATED
} ProcessState;

// Process priority levels
typedef enum {
  PROCESS_PRIORITY_LOW = 0,
  PROCESS_PRIORITY_NORMAL = 1,
  PROCESS_PRIORITY_HIGH = 2
} ProcessPriority;

// CPU context structure (i686 specific)
typedef struct {
  uint32_t eax;    // General purpose registers
  uint32_t ebx;    // General purpose registers
  uint32_t ecx;    // General purpose registers
  uint32_t edx;    // General purpose registers
  uint32_t esi;    // Index registers
  uint32_t edi;    // Index registers
  uint32_t ebp;    // Base pointer (stack frame)
  uint32_t esp;    // Stack pointer (top of stack)
  uint32_t eip;    // Instruction pointer (where to resume)
  uint32_t eflags; // CPU flags (interrupts, etc.)
  uint32_t cr3;    // Page directory base
} CpuContext;

// Process Control Block structure
typedef struct Pcb {
  uint32_t pid;             // Unique process identifier
  char name[32];            // Human-readable name
  ProcessState state;       // Current lifecycle state
  ProcessPriority priority; // Scheduling priority

  CpuContext cpu_context;

  uint32_t stack_base; // bottom of the stack (lowest address)
  uint32_t stack_size; // stack size in bytes
  uint32_t stack_top;  // current top of stack (ESP)

  uint32_t heap_start; // start of the heap
  uint32_t heap_end;   // current mapped end of heap
  uint32_t brk;        // current top of allocated heap (like UNIX brk)

  uint32_t *page_directory; // Virtual memory mapping

  uint32_t time_slice; // How long it can run (milliseconds)
  uint32_t time_used;  // Time used in current turn

  void *wait_object;     // What it's waiting for (file, mutex, etc.)
  uint32_t wait_timeout; // Max time to wait

  uint32_t parent_pid; // Who created this process
  int exit_code;       // Return value when done
} Pcb;

// PCB subsystem initialization
void pcb_init(void);

// PCB management functions
Pcb *pcb_create(uint32_t pid, const char *name, ProcessPriority priority);
void pcb_destroy(Pcb *pcb);
void pcb_context_init(Pcb *pcb, uint32_t entry_point, uint32_t stack_top);
void pcb_state_set(Pcb *pcb, ProcessState state);
const char *pcb_state_string_get(ProcessState state);
const char *pcb_priority_string_get(ProcessPriority priority);
