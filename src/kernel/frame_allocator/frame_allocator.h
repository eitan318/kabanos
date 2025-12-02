#pragma once

#include "boot/bootparams.h"
#include <stdbool.h>
#include <stdint.h>

#define PAGE_SIZE 4096

// Each free frame stores a pointer to the next free frame
typedef struct FreeFrame {
  struct FreeFrame *next;
} FreeFrame;

// Free list implementation structure
struct FrameAllocator {
  FreeFrame *free_list_head; // Head of free list
  uint64_t total_frames;     // Total number of frames
  uint64_t free_frames;      // Number of free frames
};

// Opaque allocator structure - implementation details hidden
typedef struct FrameAllocator FrameAllocator;

// Initialize the frame allocator
void frame_allocator_init(FrameAllocator *allocator, MemoryMap *mmap);

// Allocate a single frame (returns physical address or 0 on failure)
uint64_t frame_alloc(FrameAllocator *allocator);

// Free a single frame
void frame_free(FrameAllocator *allocator, uint64_t frame_addr);

// Mark a range of frames as used (for reserving kernel/hardware memory)
void frame_mark_range_used(FrameAllocator *allocator, uint64_t start_addr,
                           uint64_t end_addr);

// Get statistics
uint64_t frame_get_free_count(FrameAllocator *allocator);
uint64_t frame_get_used_count(FrameAllocator *allocator);
uint64_t frame_get_total_count(FrameAllocator *allocator);
