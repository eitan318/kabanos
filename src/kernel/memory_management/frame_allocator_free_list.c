#include "boot/bootparams.h"
#include "frame_allocator.h"
#include <string.h>

// Bitmap: 1 bit per frame
// 0 = free, 1 = used
static uint8_t *bitmap = NULL;
static uint64_t total_frames = 0;
static uint64_t used_frames = 0;
static uint64_t bitmap_size_bytes = 0;

// Memory region tracking
static uint64_t memory_start = 0;
static uint64_t memory_end = 0;

// Helper: convert physical address to frame index
static inline uint64_t addr_to_frame(uint64_t addr) {
  return (addr - memory_start) / PAGE_SIZE;
}

// Helper: convert frame index to physical address
static inline uint64_t frame_to_addr(uint64_t frame) {
  return memory_start + (frame * PAGE_SIZE);
}

// Helper: check if frame is used
static inline bool is_frame_used(uint64_t frame) {
  uint64_t byte_idx = frame / 8;
  uint8_t bit_idx = frame % 8;
  return (bitmap[byte_idx] & (1 << bit_idx)) != 0;
}

// Helper: mark frame as used
static inline void mark_frame_used(uint64_t frame) {
  uint64_t byte_idx = frame / 8;
  uint8_t bit_idx = frame % 8;
  if (!is_frame_used(frame)) {
    bitmap[byte_idx] |= (1 << bit_idx);
    used_frames++;
  }
}

// Helper: mark frame as free
static inline void mark_frame_free(uint64_t frame) {
  uint64_t byte_idx = frame / 8;
  uint8_t bit_idx = frame % 8;
  if (is_frame_used(frame)) {
    bitmap[byte_idx] &= ~(1 << bit_idx);
    used_frames--;
  }
}

void frame_allocator_init(MemoryMap *mmap) {
  // Find the largest usable memory region
  memory_start = UINT64_MAX;
  memory_end = 0;

  for (uint32_t i = 0; i < mmap->region_count; i++) {
    MemoryRegion *region = &mmap->regions[i];

    // Only consider usable memory
    if (region->type != E820_USABLE) {
      continue;
    }

    uint64_t region_start = region->base;
    uint64_t region_end = region->base + region->length;

    if (region_start < memory_start) {
      memory_start = region_start;
    }
    if (region_end > memory_end) {
      memory_end = region_end;
    }
  }

  // Align to page boundaries
  memory_start = (memory_start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  memory_end = memory_end & ~(PAGE_SIZE - 1);

  // Calculate total frames
  total_frames = (memory_end - memory_start) / PAGE_SIZE;

  // Calculate bitmap size (1 bit per frame, round up to bytes)
  bitmap_size_bytes = (total_frames + 7) / 8;

  // Place bitmap at the start of usable memory
  // This is safe because we'll mark it as used
  bitmap = (uint8_t *)memory_start;

  // Zero out the bitmap (all frames free initially)
  memset(bitmap, 0, bitmap_size_bytes);
  used_frames = 0;

  // Mark the bitmap itself as used
  uint64_t bitmap_frames = (bitmap_size_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
  for (uint64_t i = 0; i < bitmap_frames; i++) {
    mark_frame_used(i);
  }

  // Mark non-usable regions as used
  for (uint32_t i = 0; i < mmap->region_count; i++) {
    MemoryRegion *region = &mmap->regions[i];

    if (region->type != E820_USABLE) {
      // Mark this region as used
      uint64_t start = region->base;
      uint64_t end = region->base + region->length;

      // Clamp to our tracked range
      if (start < memory_start)
        start = memory_start;
      if (end > memory_end)
        end = memory_end;

      if (start < end) {
        uint64_t start_frame = addr_to_frame(start);
        uint64_t end_frame = addr_to_frame(end - 1) + 1;

        for (uint64_t f = start_frame; f < end_frame && f < total_frames; f++) {
          mark_frame_used(f);
        }
      }
    }
  }
}

void frame_mark_range_used(uint64_t start_addr, uint64_t end_addr) {
  // Align to page boundaries
  start_addr = start_addr & ~(PAGE_SIZE - 1);
  end_addr = (end_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

  // Clamp to our tracked range
  if (start_addr < memory_start)
    start_addr = memory_start;
  if (end_addr > memory_end)
    end_addr = memory_end;

  if (start_addr >= end_addr) {
    return;
  }

  uint64_t start_frame = addr_to_frame(start_addr);
  uint64_t end_frame = addr_to_frame(end_addr);

  for (uint64_t frame = start_frame; frame < end_frame && frame < total_frames;
       frame++) {
    mark_frame_used(frame);
  }
}

uint64_t frame_alloc() {
  // Scan bitmap for a free frame
  for (uint64_t frame = 0; frame < total_frames; frame++) {
    if (!is_frame_used(frame)) {
      mark_frame_used(frame);
      return frame_to_addr(frame);
    }
  }

  // Out of memory
  return 0;
}

void frame_free(uint64_t frame_addr) {
  // Reject NULL or unaligned addresses
  if (frame_addr == 0 || (frame_addr & (PAGE_SIZE - 1)) != 0) {
    return;
  }

  // Reject out-of-range addresses
  if (frame_addr < memory_start || frame_addr >= memory_end) {
    return;
  }

  uint64_t frame = addr_to_frame(frame_addr);
  if (frame >= total_frames) {
    return;
  }

  mark_frame_free(frame);
}

uint64_t frame_get_free_count() { return total_frames - used_frames; }

uint64_t frame_get_used_count() { return used_frames; }

uint64_t frame_get_total_count() { return total_frames; }
