#include "frame_allocator.h"
#include "boot/bootparams.h"
#include "include/stdio.h"
#include "utils/bitmap.h"
#include "utils/math.h"
#include <locale.h>

#define MAX_FRAMES (1024 * 1024)               // 4GB worth of 4KB frames
#define MAX_BITMAP_SIZE ((MAX_FRAMES + 7) / 8) // 128KB

// Static bitmap in BSS section (automatically zeroed)
static uint8_t bitmap[MAX_BITMAP_SIZE];
static uint64_t total_frames = 0;
static uint64_t used_frames = 0;
static uint64_t bitmap_size_bytes = 0;

static Range memory_range;

static Range find_range(MemoryMap *memory_map) {
  uint64_t max_addr = 0;          // Initialize to 0
  uint64_t min_addr = UINT64_MAX; // Initialize to max value

  for (int i = 0; i < memory_map->region_count; i++) {
    MemoryRegion *region = &memory_map->regions[i];

    if (region->type != E820_USABLE)
      continue;

    uint64_t region_start = region->base;
    uint64_t region_end = region->base + region->length;

    if (region_start < min_addr)
      min_addr = region_start;
    if (region_end > max_addr)
      max_addr = region_end;
  }

  Range range = {
      .start = min_addr,
      .end = max_addr,
  };

  return range;
}

static void mark_non_usable_ranges_as_used(MemoryMap *memory_map) {
  for (int i = 0; i < memory_map->region_count; i++) {
    MemoryRegion *region = &memory_map->regions[i];
    if (region->type == E820_USABLE)
      continue;

    Range region_range = {
        .start = region->base,
        .end = region->base + region->length,
    };
    region_range = range_align_outward(region_range, PAGE_SIZE);
    region_range = range_clamp(region_range, memory_range);

    frame_mark_range_used(region_range);
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

// Changed return type to uint64_t to match total_frames type
static inline uint64_t aligned_addr_to_frame(uint64_t addr) {
  return (addr - memory_range.start) / PAGE_SIZE;
}

static inline uint64_t frame_to_aligned_addr(uint64_t frame) {
  return memory_range.start + (frame * PAGE_SIZE);
}

static inline void mark_frame_used(uint64_t frame) {
  if (!bitmap_test(bitmap, frame)) {
    bitmap_set(bitmap, frame);
    used_frames++;
  }
}

static void mark_frame_free(uint64_t frame) {
  if (bitmap_test(bitmap, frame)) {
    bitmap_clear(bitmap, frame);
    used_frames--;
  }
}

void frame_mark_range_used(Range range) {
  range = range_align_outward(range, PAGE_SIZE);
  range = range_clamp(range, memory_range);

  if (range.start >= range.end) {
    return;
  }

  // FIXED: start_frame should use range.start, not range.end
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
    if (!bitmap_test(bitmap, frame)) {
      mark_frame_used(frame);
      return frame_to_aligned_addr(frame);
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

  if (!in_range(frame_addr, memory_range)) {
    return;
  }

  uint64_t frame = aligned_addr_to_frame(frame_addr);
  if (frame >= total_frames) {
    return;
  }

  mark_frame_free(frame);
}

void frame_allocator_init(BootParams *boot_params) {
  memory_range = find_range(&boot_params->memory_map);
  memory_range = range_align_outward(memory_range, PAGE_SIZE);

  total_frames = (memory_range.end - memory_range.start) / PAGE_SIZE;
  if (total_frames > MAX_FRAMES) {
    debugf("Memory range greater than max frames!");
    total_frames = MAX_FRAMES;
    memory_range.end = memory_range.start + (MAX_FRAMES * PAGE_SIZE);
  }

  bitmap_size_bytes = (total_frames + 7) / 8;

  used_frames = 0;
  mark_frame_used(0); // for alloc to return 0 on err, frame 0 must be used

  mark_non_usable_ranges_as_used(&boot_params->memory_map);
  mark_kernel_as_used();
  mark_initrd_as_used(boot_params);
  mark_modules_as_used(boot_params);
  mark_cmdline_buffer_as_used(boot_params);
}

// -1 because the real count of usable frames does not include first frame
uint64_t frame_get_free_count() { return total_frames - 1 - used_frames; }
uint64_t frame_get_used_count() { return used_frames; }

// -1 because the real count of usable frames does not include first frame
uint64_t frame_get_total_count() { return total_frames - 1; }
