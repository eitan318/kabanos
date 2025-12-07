#include "pcb.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "include/string.h"
#include "kmalloc.h"

static uint32_t next_pid = 1;

Pcb *pcb_create(uint32_t pid, const char *name, ProcessPriority priority) {
  Pcb *pcb = (Pcb *)kmalloc(sizeof(Pcb));
  if (!pcb) {
    return NULL;
  }

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
  pcb->time_slice = 10; // Default time slice
  pcb->time_used = 0;

  pcb->wait_object = NULL;
  pcb->wait_timeout = 0;
  pcb->parent_pid = 0;
  pcb->exit_code = 0;

  return pcb;
}

void pcb_context_init(Pcb *pcb, uint32_t entry_point, uint32_t stack_top) {
  if (!pcb) {
    return;
  }

  memset(&pcb->cpu_context, 0, sizeof(CpuContext));

  pcb->cpu_context.eip = entry_point;
  pcb->cpu_context.esp = stack_top;
  pcb->cpu_context.ebp = stack_top;
  pcb->cpu_context.eflags = 0x202; // Enable interrupts
}

void pcb_destroy(Pcb *pcb) {
  if (!pcb) {
    return;
  }

  if (pcb->stack_top) {
    kfree((void *)pcb->stack_top);
  }

  kfree(pcb);
}

void pcb_state_set(Pcb *pcb, ProcessState state) {
  if (pcb) {
    pcb->state = state;
  }
}

void pcb_priority_set(Pcb *pcb, ProcessPriority priority) {
  if (pcb) {
    pcb->priority = priority;
  }
}

// Debug helpers
const char *pcb_state_string_get(ProcessState state) {
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

const char *pcb_priority_string_get(ProcessPriority priority) {
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
