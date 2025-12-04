#include "pcb.h"
#include "include/string.h"
#include "include/stdio.h"
#include "include/memory.h"

// DO NOT initialize - let it go in BSS (will be zeroed)
static uint32_t next_pid;
static bool pcb_initialized;

// Initialize PCB subsystem - call this before using PCBs
void pcb_init(void) {
    if (pcb_initialized) {
        return;
    }
    next_pid = 1;
    pcb_initialized = true;
}

// Create a new PCB
Pcb* pcb_create(uint32_t pid, const char* name, ProcessPriority priority) {
    // Auto-initialize on first use
    if (!pcb_initialized) {
        pcb_init();
    }
    
    Pcb* pcb = (Pcb*)kmalloc(sizeof(Pcb));
    if (!pcb) {
        return NULL;
    }
    
    // Initialize PCB fields
    memset(pcb, 0, sizeof(Pcb));
    
    pcb->pid = (pid == 0) ? next_pid++ : pid;
    
    // Copy process name
    if (name) {
        strncpy(pcb->name, name, sizeof(pcb->name) - 1);
        pcb->name[sizeof(pcb->name) - 1] = '\0';
    } else {
        snprintf(pcb->name, sizeof(pcb->name), "process_%u", pcb->pid);
    }
    
    pcb->state = PROCESS_STATE_NEW;
    pcb->priority = priority;
    pcb->time_slice = 10;  // Default time slice
    pcb->time_used = 0;
    
    // Removed next/prev initialization - not in struct anymore
    pcb->wait_object = NULL;
    pcb->wait_timeout = 0;
    pcb->parent_pid = 0;
    pcb->exit_code = 0;
    
    return pcb;
}

// Destroy a PCB and free its resources
void pcb_destroy(Pcb* pcb) {
    if (!pcb) {
        return;
    }
    
    // Free stack if allocated
    if (pcb->stack_base) {
        kfree(pcb->stack_base);
    }
    
    // Free the PCB itself
    kfree(pcb);
}

// Initialize CPU context for a new process
void pcb_context_init(Pcb* pcb, uint32_t entry_point, uint32_t stack_top) {
    if (!pcb) {
        return;
    }
    
    memset(&pcb->context, 0, sizeof(CpuContext));
    
    pcb->context.eip = entry_point;
    pcb->context.esp = stack_top;
    pcb->context.ebp = stack_top;
    pcb->context.eflags = 0x202;  // Enable interrupts flag
}

// Set process state
void pcb_state_set(Pcb* pcb, ProcessState state) {
    if (pcb) {
        pcb->state = state;
    }
}

// Convert state to string for debugging
const char* pcb_state_string_get(ProcessState state) {
    switch (state) {
        case PROCESS_STATE_NEW:
            return "NEW";
        case PROCESS_STATE_READY:
            return "READY";
        case PROCESS_STATE_RUNNING:
            return "RUNNING";
        case PROCESS_STATE_WAITING:
            return "WAITING";
        case PROCESS_STATE_TERMINATED:
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}

// Convert priority to string for debugging
const char* pcb_priority_string_get(ProcessPriority priority) {
    switch (priority) {
        case PROCESS_PRIORITY_LOW:
            return "LOW";
        case PROCESS_PRIORITY_NORMAL:
            return "NORMAL";
        case PROCESS_PRIORITY_HIGH:
            return "HIGH";
        default:
            return "UNKNOWN";
    }
}