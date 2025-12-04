#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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
    uint32_t cr3;  // Page directory base
} CpuContext;

// Process Control Block structure
typedef struct Pcb {
    uint32_t pid;                    // Process ID
    char name[32];                   // Process name
    ProcessState state;              // Current state
    ProcessPriority priority;        // Process priority
    
    CpuContext context;              // Saved CPU context
    
    uint32_t* stack_base;            // Stack base address
    uint32_t stack_size;             // Stack size
    
    uint32_t* page_directory;        // Pointer to page directory
    
    uint32_t time_slice;             // Time quantum for scheduling
    uint32_t time_used;              // Time used in current slice
    
    void* wait_object;               // Object process is waiting for (if any)
    uint32_t wait_timeout;           // Timeout for waiting
    
    uint32_t parent_pid;             // Parent process ID
    int exit_code;                   // Exit code when terminated
} Pcb;

// PCB subsystem initialization
void pcb_init(void);

// PCB management functions
Pcb* pcb_create(uint32_t pid, const char* name, ProcessPriority priority);
void pcb_destroy(Pcb* pcb);
void pcb_context_init(Pcb* pcb, uint32_t entry_point, uint32_t stack_top);
void pcb_state_set(Pcb* pcb, ProcessState state);
const char* pcb_state_string_get(ProcessState state);
const char* pcb_priority_string_get(ProcessPriority priority);