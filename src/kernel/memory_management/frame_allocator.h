#pragma once

#include "boot/bootparams.h"
#include "utils/range.h"
#include <stdbool.h>

#include <stdint.h>

#define PAGE_SIZE 4096

// Initialize the frame allocator
void frame_allocator_init(BootParams *boot_params);

// Allocate a single frame (returns physical address or 0 on failure)
uint64_t frame_alloc();

// Free a single frame
void frame_free(uint64_t frame_addr);

// Mark a range of frames as used (for reserving kernel/hardware memory)
void frame_mark_range_used(Range range);

uint64_t frame_get_free_count();
