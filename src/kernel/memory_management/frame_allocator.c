#include "frame_allocator.h"
#include "boot/bootparams.h"
#include "include/string.h"

// Maximum memory we can track (e.g., 4GB = 1M frames = 128KB bitmap)
// Adjust this based on your expected maximum RAM
#define MAX_FRAMES (1024 * 1024)               // 4GB worth of 4KB frames
#define MAX_BITMAP_SIZE ((MAX_FRAMES + 7) / 8) // 128KB

// Static bitmap in BSS section (automatically zeroed)
static uint8_t bitmap[MAX_BITMAP_SIZE];
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

void frame_allocator_init(BootParams *boot_params) {
  // Find the total usable memory range
  memory_start = UINT64_MAX;
  memory_end = 0;
  MemoryMap *mmap = &boot_params->memory_map;

  for (uint32_t i = 0; i < mmap->region_count; i++) {
    MemoryRegion *region = &mmap->regions[i];
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

  // Check if we can track this much memory
  if (total_frames > MAX_FRAMES) {
    // Handle error or reduce tracking
    total_frames = MAX_FRAMES;
    memory_end = memory_start + (MAX_FRAMES * PAGE_SIZE);
  }

  // Calculate actual bitmap size needed
  bitmap_size_bytes = (total_frames + 7) / 8;

  // Bitmap is already zeroed (it's in BSS)
  used_frames = 0;

  // Mark non-usable regions as used
  for (uint32_t i = 0; i < mmap->region_count; i++) {
    MemoryRegion *region = &mmap->regions[i];
    if (region->type != E820_USABLE) {
      uint64_t start = region->base;
      uint64_t end = region->base + region->length;

      // Align to page boundaries
      start = start & ~(PAGE_SIZE - 1);
      end = (end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

      // Clamp to our tracked range
      if (start < memory_start)
        start = memory_start;
      if (end > memory_end)
        end = memory_end;

      if (start < end) {
        uint64_t start_frame = addr_to_frame(start);
        uint64_t end_frame = addr_to_frame(end);

        for (uint64_t f = start_frame; f < end_frame && f < total_frames; f++) {
          mark_frame_used(f);
        }
      }
    }
  }

  // Mark kernel as used
  extern uint8_t _kernel_start, _kernel_end;
  frame_mark_range_used((uint64_t)&_kernel_start, (uint64_t)&_kernel_end);

  // Mark initrd as used
  if (boot_params->initrd_start && boot_params->initrd_size > 0) {
    frame_mark_range_used((uint64_t)boot_params->initrd_start,
                          (uint64_t)boot_params->initrd_start +
                              boot_params->initrd_size);
  }

  // Mark all modules as used
  for (uint32_t i = 0; i < boot_params->module_count; i++) {
    Module *mod = &boot_params->modules[i];
    if (mod->start && mod->size > 0) {
      frame_mark_range_used((uint64_t)mod->start,
                            (uint64_t)mod->start + mod->size);
    }
  }

  // Mark command line buffer as used
  if (boot_params->cmdline_buffer && boot_params->cmdline_size > 0) {
    frame_mark_range_used((uint64_t)boot_params->cmdline_buffer,
                          (uint64_t)boot_params->cmdline_buffer +
                              boot_params->cmdline_size);
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
  for (uint64_t frame = 1; frame < total_frames; frame++) {
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
