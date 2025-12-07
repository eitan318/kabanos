#pragma once
#include "pcb.h"
#include "memory_management/paging.h"
#include <stdbool.h>
#include <stdint.h>

// Default memory layout for processes
#define PROCESS_HEAP_START   0x08000000  // 128MB
#define PROCESS_HEAP_SIZE    0x00100000  // 1MB
#define PROCESS_STACK_TOP    0xBFFFF000  // Just below 3GB
#define PROCESS_STACK_SIZE   0x00002000  // 8KB

// Process management
void process_init(void);
bool process_register(Pcb *pcb);
bool process_unregister(uint32_t pid);
void process_list_all(void);

// Process lifecycle
Pcb *process_create(const char *elf_path);
void process_kill(Pcb *pcb);

// Memory management helpers
bool allocate_heap(Pcb *pcb, void *heap_start, uint32_t heap_size,
                   PageDirectory *page_dir);
bool allocate_stack(Pcb *pcb, uint32_t stack_top, uint32_t stack_size,
                    PageDirectory *page_dir);
void free_heap(Pcb *pcb, PageDirectory *page_dir);
void free_stack(Pcb *pcb, PageDirectory *page_dir);