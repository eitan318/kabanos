#include "frame_allocator.h"
#include "boot/bootparams.h"
#include "include/string.h"
#include "utils/range.h"
#include <stdlib.h>

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
static Range memory_range = {
    .start = 0,
    .end = 0,
};

// Helper: convert physical address to frame index
static inline uint64_t aligned_addr_to_frame(uint64_t addr) {
  return (addr - memory_range.start) / PAGE_SIZE;
}

// Helper: convert frame index to physical address
static inline uint64_t frame_to_addr(uint64_t frame) {
  return memory_range.start + (frame * PAGE_SIZE);
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
static void mark_kernel_as_used() {
  extern uint8_t _kernel_start, _kernel_end;

  Range kernel_range = {.start = (uint64_t)&_kernel_start,
                        .end = (uint64_t)&_kernel_end};

  frame_mark_range_used(kernel_range);
}

static void mark_initrd_as_used(BootParams *boot_params) {
  if (boot_params->initrd_start && boot_params->initrd_size > 0) {
    Range initrd_range = {.start = (uint64_t)boot_params->initrd_start,
                          .end = (uint64_t)boot_params->initrd_start +
                                 boot_params->initrd_size};
    frame_mark_range_used(initrd_range);
  }
}

static void mark_modules_as_used(BootParams *boot_params) {
  for (uint32_t i = 0; i < boot_params->module_count; i++) {
    Module *mod = &boot_params->modules[i];
    if (mod->start && mod->size > 0) {
      Range mod_range = {.start = (uint64_t)mod->start,
                         .end = (uint64_t)mod->start + mod->size};
      frame_mark_range_used(mod_range);
    }
  }
}

static void mark_cmdline_buffer_as_used(BootParams *boot_params) {
  if (boot_params->cmdline_buffer && boot_params->cmdline_size > 0) {
    Range cmdline_range = {.start = (uint64_t)boot_params->cmdline_buffer,
                           .end = (uint64_t)boot_params->cmdline_buffer +
                                  boot_params->cmdline_size};
    frame_mark_range_used(cmdline_range);
  }
}

void frame_allocator_init(BootParams *boot_params) {
  // Find the total usable memory range
  memory_range.start = UINT64_MAX;
  memory_range.end = 0;
  MemoryMap *mmap = &boot_params->memory_map;

  for (uint32_t i = 0; i < mmap->region_count; i++) {
    MemoryRegion *region = &mmap->regions[i];
    if (region->type != E820_USABLE) {
      continue;
    }

    uint64_t region_start = region->base;
    uint64_t region_end = region->base + region->length;

    if (region_start < memory_range.start) {
      memory_range.start = region_start;
    }
    if (region_end > memory_range.end) {
      memory_range.end = region_end;
    }
  }

  // Align to page boundaries
  memory_range.start = (memory_range.start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  memory_range.end = memory_range.end & ~(PAGE_SIZE - 1);

  // Calculate total frames
  total_frames = (memory_range.end - memory_range.start) / PAGE_SIZE;

  // Check if we can track this much memory
  if (total_frames > MAX_FRAMES) {
    // Handle error or reduce tracking
    total_frames = MAX_FRAMES;
    memory_range.end = memory_range.start + (MAX_FRAMES * PAGE_SIZE);
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
      if (start < memory_range.start)
        start = memory_range.start;
      if (end > memory_range.end)
        end = memory_range.end;

      if (start < end) {
        uint64_t start_frame = aligned_addr_to_frame(start);
        uint64_t end_frame = aligned_addr_to_frame(end);

        for (uint64_t f = start_frame; f < end_frame && f < total_frames; f++) {
          mark_frame_used(f);
        }
      }
    }
  }

  // Mark kernel as used
  mark_kernel_as_used();
  mark_initrd_as_used(boot_params);
  mark_modules_as_used(boot_params);
  mark_cmdline_buffer_as_used(boot_params);
}

void frame_mark_range_used(Range range) {
  // Align to page boundaries
  range.start = range.start & ~(PAGE_SIZE - 1);
  range.end = (range.end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

  // Clamp to our tracked range
  if (range.start < memory_range.start)
    range.start = memory_range.start;
  if (range.end > memory_range.end)
    range.end = memory_range.end;

  if (range.start >= range.end) {
    return;
  }

  uint64_t start_frame = aligned_addr_to_frame(range.start);
  uint64_t end_frame = aligned_addr_to_frame(range.end);

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
  if (frame_addr < memory_range.start || frame_addr >= memory_range.end) {
    return;
  }

  uint64_t frame = aligned_addr_to_frame(frame_addr);
  if (frame >= total_frames) {
    return;
  }

  mark_frame_free(frame);
}

uint64_t frame_get_free_count() { return total_frames - used_frames; }

uint64_t frame_get_used_count() { return used_frames; }

uint64_t frame_get_total_count() { return total_frames; }
