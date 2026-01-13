#include "pmm.h"
#include "include/stdio.h"
#include "utils/bitmap.h"
#include "utils/math.h"
#include "utils/range.h"
#include <string.h>

#define MAX_FRAMES (1024 * 1024)               // 4GB worth of 4KB frames
#define MAX_BITMAP_SIZE ((MAX_FRAMES + 7) / 8) // 128KB

// Static bitmap in BSS section (automatically zeroed)
static uint8_t bitmap[MAX_BITMAP_SIZE];
static uint64_t total_frames = 0;
static uint64_t used_frames = 0;
static uint64_t bitmap_size_bytes = 0;
static bool initialized = false;
static Range memory_range;

static inline void mark_frame_used(uint64_t frame) {
  if (!bitmap_test(bitmap, frame)) {
    bitmap_set(bitmap, frame);
    used_frames++;
  }
}

static inline uint64_t aligned_addr_to_frame(uint64_t addr) {
  return (addr - memory_range.start) / FRAME_SIZE;
}

void pmm_mark_range_used(paddr_t from, paddr_t to) {
  Range range = {.start = from, .end = to};
  range = range_align_outward(range, FRAME_SIZE);
  range = range_clamp(range, memory_range);

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
static inline uint64_t frame_to_aligned_addr(uint64_t frame) {
  return memory_range.start + (frame * FRAME_SIZE);
}

static void mark_frame_free(uint64_t frame) {
  if (bitmap_test(bitmap, frame)) {
    bitmap_clear(bitmap, frame);
    used_frames--;
  }
}

uint64_t pmm_frame_alloc() {
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

void pmm_frame_free(uint64_t frame_addr) {
  // Reject NULL or unaligned addresses
  if (frame_addr == 0 || (frame_addr & (FRAME_SIZE - 1)) != 0) {
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

void pmm_init(Range total_memory_range, Range *used_ranges,
              int used_ranges_count) {
  memory_range = range_align_outward(total_memory_range, FRAME_SIZE);
  total_frames = (memory_range.end - memory_range.start) / FRAME_SIZE;
  if (total_frames > MAX_FRAMES) {
    debugf("Memory range greater than max frames!");
    total_frames = MAX_FRAMES;
    memory_range.end = memory_range.start + ((uint64_t)MAX_FRAMES * FRAME_SIZE);
  }

  bitmap_size_bytes = align_up(total_frames, 8);
  memset(bitmap, 0, bitmap_size_bytes);

  used_frames = 0;
  mark_frame_used(0); // for alloc to return 0 on err, frame 0 must be used
  for (int i = 0; i < used_ranges_count; i++) {
    pmm_mark_range_used(used_ranges[i].start, used_ranges[i].end);
  }
}

// -1 because the real count of usable frames does not include first frame
uint64_t frame_get_free_count() { return total_frames - 1 - used_frames; }
uint64_t frame_get_used_count() { return used_frames; }

// -1 because the real count of usable frames does not include first frame
uint64_t frame_get_total_count() { return total_frames - 1; }
