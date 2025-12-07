#include "process/process.h"
#include "include/stdio.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"
#include "process/pcb.h"
#include "elf/elf.h"
#include "fat/fat.h"
#include "include/string.h"
#include <stddef.h>

#define MAX_PROCESSES 256

static Pcb *process_table[MAX_PROCESSES];
static uint32_t process_count = 0;

static bool initialized = false;

void process_init(void) {
  if (initialized) {
    return;
  }

  for (int i = 0; i < MAX_PROCESSES; i++) {
    process_table[i] = NULL;
  }
  process_count = 0;
  initialized = true;
}

bool process_register(Pcb *pcb) {
  if (!pcb || !initialized) {
    return false;
  }
  if (pcb->pid >= MAX_PROCESSES || pcb->pid == 0) {
    return false;
  }
  if (process_table[pcb->pid] != NULL) {
    return false;
  }

  process_table[pcb->pid] = pcb;
  process_count++;
  return true;
}

bool process_unregister(uint32_t pid) {
  if (!initialized) {
    return false;
  }
  if (pid >= MAX_PROCESSES || pid == 0) {
    return false;
  }
  if (process_table[pid] == NULL) {
    return false;
  }
  process_table[pid] = NULL;
  process_count--;
  return true;
}

void process_list_all(void) {
  printf("Process List (%u processes):\n", process_count);
  printf("PID\tName\t\t\tState\t\tPriority\n");
  printf("---\t----\t\t\t-----\t\t--------\n");

  for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
    Pcb *pcb = process_table[i];
    if (pcb != NULL) {
      printf("%u\t%-20s\t%-10s\t%s\n", pcb->pid, pcb->name,
             pcb_state_string_get(pcb->state),
             pcb_priority_string_get(pcb->priority));
    }
  }
}

void free_heap(Pcb *pcb, PageDirectory *page_dir) {
  if (!pcb || pcb->heap_start == 0) {
    return;
  }

  uint32_t heap_size = pcb->heap_end - pcb->heap_start;
  uint32_t pages_needed = (heap_size + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t virt_addr = pcb->heap_start;

  for (uint32_t i = 0; i < pages_needed; i++) {
    uint32_t phys = paging_get_physical(page_dir, virt_addr);

    if (phys != 0) {
      paging_unmap(page_dir, virt_addr);
      frame_free(phys);
    }

    virt_addr += PAGE_SIZE;
  }

  pcb->heap_start = 0;
  pcb->heap_end = 0;
  pcb->brk = 0;
}

void free_stack(Pcb *pcb, PageDirectory *page_dir) {
  if (!pcb || pcb->stack_top == 0) {
    return;
  }

  uint32_t pages_needed = (pcb->stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t stack_bottom = pcb->stack_top;
  uint32_t stack_top = stack_bottom + pcb->stack_size;
  uint32_t virt_addr = stack_top - PAGE_SIZE; // Start from top page

  for (uint32_t i = 0; i < pages_needed; i++) {
    uint32_t phys = paging_get_physical(page_dir, virt_addr);

    if (phys != 0) {
      paging_unmap(page_dir, virt_addr);
      frame_free(phys);
    }

    virt_addr -= PAGE_SIZE;
  }

  pcb->stack_top = 0;
  pcb->stack_size = 0;
  pcb->cpu_context.esp = 0;
  pcb->cpu_context.ebp = 0;
}

bool allocate_heap(Pcb *pcb, void *heap_start, uint32_t heap_size,
                   PageDirectory *page_dir) {
  if (!pcb || !heap_start || !page_dir) {
    return false;
  }

  uint32_t pages_needed = (heap_size + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t virt_addr = (uint32_t)heap_start;
  uint32_t pages_allocated = 0;

  for (uint32_t i = 0; i < pages_needed; i++) {
    uint32_t phys = frame_alloc();
    if (phys == 0) {
      debugf("Frame allocation failed in allocate_heap at page %u\n", i);
      goto cleanup_on_error;
    }

    if (!paging_map(page_dir, virt_addr, phys, PAGE_USER | PAGE_WRITABLE)) {
      debugf("Mapping failure in allocate_heap at virt=0x%x\n", virt_addr);
      frame_free(phys); // Free the frame we just allocated
      goto cleanup_on_error;
    }

    pages_allocated++;
    virt_addr += PAGE_SIZE;
  }

  pcb->heap_start = (uint32_t)heap_start;
  pcb->heap_end = (uint32_t)heap_start + heap_size;
  pcb->brk = (uint32_t)heap_start;
  return true;

cleanup_on_error:
  // Roll back partial allocation
  virt_addr = (uint32_t)heap_start;
  for (uint32_t i = 0; i < pages_allocated; i++) {
    uint32_t phys = paging_get_physical(page_dir, virt_addr);
    if (phys != 0) {
      paging_unmap(page_dir, virt_addr);
      frame_free(phys);
    }
    virt_addr += PAGE_SIZE;
  }
  return false;
}

bool allocate_stack(Pcb *pcb, uint32_t stack_top, uint32_t stack_size,
                    PageDirectory *page_dir) {
  if (!pcb || !page_dir || stack_size == 0) {
    return false;
  }

  uint32_t pages_needed = (stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t virt_addr = stack_top - PAGE_SIZE; // Start at top - one page
  uint32_t pages_allocated = 0;

  for (uint32_t i = 0; i < pages_needed; i++) {
    uint32_t phys = frame_alloc();
    if (phys == 0) {
      debugf("Frame allocation failed in allocate_stack at page %u\n", i);
      goto cleanup_on_error;
    }

    if (!paging_map(page_dir, virt_addr, phys, PAGE_USER | PAGE_WRITABLE)) {
      debugf("Mapping failure in allocate_stack at virt=0x%x\n", virt_addr);
      frame_free(phys); // Free the frame we just allocated
      goto cleanup_on_error;
    }

    pages_allocated++;
    virt_addr -= PAGE_SIZE;
  }

  // Success - set PCB metadata
  pcb->cpu_context.esp = stack_top;
  pcb->cpu_context.ebp = stack_top;
  pcb->stack_top = stack_top - stack_size;
  pcb->stack_size = stack_size;
  return true;

cleanup_on_error:
  // Roll back partial allocation
  virt_addr = stack_top - PAGE_SIZE;
  for (uint32_t i = 0; i < pages_allocated; i++) {
    uint32_t phys = paging_get_physical(page_dir, virt_addr);
    if (phys != 0) {
      paging_unmap(page_dir, virt_addr);
      frame_free(phys);
    }
    virt_addr -= PAGE_SIZE;
  }
  return false;
}

void process_kill(Pcb *pcb) {
  if (!pcb) {
    return;
  }

  uint32_t pid = pcb->pid;
  pcb_state_set(pcb, PROCESS_STATE_TERMINATED);
  process_unregister(pid);
  pcb_destroy(pcb);
}

/**
 * Create a new process from an ELF file
 * 
 * @param elf_path Path to ELF executable (e.g., "/calc.elf")
 * @return Pointer to created PCB, or NULL on failure
 */
Pcb *process_create(const char *elf_path) {
  if (!elf_path || !initialized) {
    debugf("ERROR: Invalid elf_path or process system not initialized\n");
    return NULL;
  }

  debugf("Creating process from '%s'...\n", elf_path);

  // Step 1: Extract process name from path (e.g., "/calc.elf" -> "calc")
  const char *name_start = strrchr(elf_path, '/');
  if (name_start) {
    name_start++; // Skip the '/'
  } else {
    name_start = elf_path;
  }

  // Remove .elf extension if present
  char process_name[32];
  strncpy(process_name, name_start, sizeof(process_name) - 1);
  process_name[sizeof(process_name) - 1] = '\0';
  
  char *dot = strchr(process_name, '.');
  if (dot) {
    *dot = '\0';
  }

  // Step 2: Allocate PCB (auto-assigns PID)
  Pcb *pcb = pcb_create(0, process_name, PROCESS_PRIORITY_NORMAL);
  if (!pcb) {
    debugf("ERROR: Failed to allocate PCB\n");
    return NULL;
  }

  debugf("  Allocated PCB (PID %u)\n", pcb->pid);

  // Step 3: Create page directory for this process
  PageDirectory *page_dir = paging_create();
  if (!page_dir) {
    debugf("ERROR: Failed to create page directory\n");
    pcb_destroy(pcb);
    return NULL;
  }

  pcb->page_directory = (uint32_t *)page_dir;
  debugf("  Created page directory\n");

  // Step 4: Identity map kernel space (0-128MB) so kernel code is accessible
  // This allows the process to execute kernel code (interrupts, syscalls, etc.)
  debugf("  Identity mapping kernel space...\n");
  for (uint32_t addr = 0; addr < 0x8000000; addr += PAGE_SIZE) {
    if (!paging_map(page_dir, addr, addr, PAGE_WRITABLE)) {
      debugf("ERROR: Failed to identity map kernel at 0x%x\n", addr);
      paging_destroy(page_dir);
      pcb_destroy(pcb);
      return NULL;
    }
  }

  // Step 5: Load ELF file → get entry point
  debugf("  Loading ELF file...\n");
  void *entry_point = elf_load(page_dir, elf_path);
  if (!entry_point) {
    debugf("ERROR: Failed to load ELF file '%s'\n", elf_path);
    paging_destroy(page_dir);
    pcb_destroy(pcb);
    return NULL;
  }

  debugf("  Entry point: 0x%x\n", (uint32_t)entry_point);

  // Step 6: Allocate heap (1MB at 0x08000000)
  debugf("  Allocating heap (%u KB at 0x%x)...\n", 
         PROCESS_HEAP_SIZE / 1024, PROCESS_HEAP_START);
  
  if (!allocate_heap(pcb, (void *)PROCESS_HEAP_START, PROCESS_HEAP_SIZE, page_dir)) {
    debugf("ERROR: Failed to allocate heap\n");
    paging_destroy(page_dir);
    pcb_destroy(pcb);
    return NULL;
  }

  // Step 7: Allocate stack (8KB at 0xBFFFF000)
  debugf("  Allocating stack (%u KB at 0x%x)...\n", 
         PROCESS_STACK_SIZE / 1024, PROCESS_STACK_TOP);
  
  if (!allocate_stack(pcb, PROCESS_STACK_TOP, PROCESS_STACK_SIZE, page_dir)) {
    debugf("ERROR: Failed to allocate stack\n");
    free_heap(pcb, page_dir);
    paging_destroy(page_dir);
    pcb_destroy(pcb);
    return NULL;
  }

  // Step 8: Initialize CPU context
  pcb_context_init(pcb, (uint32_t)entry_point, PROCESS_STACK_TOP);
  
  // Initialize all general-purpose registers to 0
  pcb->cpu_context.eax = 0;
  pcb->cpu_context.ebx = 0;
  pcb->cpu_context.ecx = 0;
  pcb->cpu_context.edx = 0;
  pcb->cpu_context.esi = 0;
  pcb->cpu_context.edi = 0;
  
  // EFLAGS: 0x200 = interrupts enabled (bit 9)
  pcb->cpu_context.eflags = 0x200;
  
  // CR3: page directory physical address
  pcb->cpu_context.cr3 = (uint32_t)page_dir;

  debugf("  Initialized CPU context:\n");
  debugf("    EIP=0x%x, ESP=0x%x, EFLAGS=0x%x\n", 
         pcb->cpu_context.eip, pcb->cpu_context.esp, pcb->cpu_context.eflags);

  // Step 9: Set state to NEW
  pcb_state_set(pcb, PROCESS_STATE_NEW);

  // Step 10: Register process and transition to READY
  if (!process_register(pcb)) {
    debugf("ERROR: Failed to register process (PID %u already exists?)\n", pcb->pid);
    free_stack(pcb, page_dir);
    free_heap(pcb, page_dir);
    paging_destroy(page_dir);
    pcb_destroy(pcb);
    return NULL;
  }

  // Transition to READY state (ready to be scheduled)
  pcb_state_set(pcb, PROCESS_STATE_READY);

  debugf("Process '%s' (PID %u) created successfully!\n", pcb->name, pcb->pid);
  debugf("  State: %s\n", pcb_state_string_get(pcb->state));
  debugf("  Priority: %s\n", pcb_priority_string_get(pcb->priority));
  debugf("  Heap: 0x%x - 0x%x (%u KB)\n", 
         pcb->heap_start, pcb->heap_end, (pcb->heap_end - pcb->heap_start) / 1024);
  debugf("  Stack: 0x%x - 0x%x (%u KB)\n", 
         pcb->stack_top, pcb->stack_top + pcb->stack_size, pcb->stack_size / 1024);

  return pcb;
}