#pragma once

#include "boot/bootparams.h"
#include <stdbool.h>
#include <stdint.h>

#define PAGE_SIZE 4096

// Each free frame stores a pointer to the next free frame
typedef struct FreeFrame {
  struct FreeFrame *next;
} FreeFrame;

// Initialize the frame allocator
void frame_allocator_init(BootParams *boot_params);

// Allocate a single frame (returns physical address or 0 on failure)
uint64_t frame_alloc();

// Free a single frame
void frame_free(uint64_t frame_addr);

// Mark a range of frames as used (for reserving kernel/hardware memory)
void frame_mark_range_used(uint64_t start_addr, uint64_t end_addr);

// Get statistics
uint64_t frame_get_free_count();
uint64_t frame_get_used_count();
uint64_t frame_get_total_count();
