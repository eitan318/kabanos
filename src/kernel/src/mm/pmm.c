#include "mm/pmm.h"
#include "adt/bitmap.h"
#include "adt/range.h"
#include "assert.h"
#include "klib/stdio.h"
#include "utils/math.h"
#include <string.h>

#define MAX_FRAMES (1024 * 1024)               // 4GB worth of 4KB frames
#define MAX_BITMAP_SIZE ((MAX_FRAMES + 7) / 8) // 128KB

static uint8_t bitmap[MAX_BITMAP_SIZE];
static uint64_t total_frames = 0;
static uint64_t used_frames = 0;
static uint64_t bitmap_size_bytes = 0;
static bool initialized = false;
static Range memory_range;
static uint16_t frame_refcounts[MAX_FRAMES];

void pmm_frame_refcount_inc(paddr_t frame_addr) {
  uint32_t frame_idx = frame_addr / FRAME_SIZE;
  if (frame_idx < total_frames) {
    frame_refcounts[frame_idx]++;
  }
}

uint16_t pmm_frame_refcount_get(paddr_t frame_addr) {
  uint32_t frame_idx = frame_addr / FRAME_SIZE;
  return (frame_idx < total_frames) ? frame_refcounts[frame_idx] : 0;
}

static inline void mark_frame_used(uint64_t frame_idx) {
  if (!bitmap_test(bitmap, frame_idx)) {
    bitmap_set(bitmap, frame_idx);
    used_frames++;
    frame_refcounts[frame_idx] = 1;
  }
}

static inline uint64_t aligned_addr_to_frame(uint64_t addr) {
  ASSERT((addr & (FRAME_SIZE - 1)) == 0);
  ASSERT(addr >= memory_range.start);
  return (addr - memory_range.start) / FRAME_SIZE;
}

void pmm_mark_range_used(paddr_t from, paddr_t to) {
  Range range = {.start = from, .end = to};
  range = range_align_outward(range, FRAME_SIZE);
  range = range_clamp(range, memory_range);

  if (range.start >= range.end)
    return;

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
  if (!bitmap_test(bitmap, frame))
    return;

  if (frame_refcounts[frame] > 0)
    frame_refcounts[frame]--;

  if (frame_refcounts[frame] == 0) {
    bitmap_clear(bitmap, frame);
    used_frames--;
  }
}

uint64_t pmm_frame_alloc() {
  for (uint64_t frame = 1; frame < total_frames; frame++) {
    if (!bitmap_test(bitmap, frame)) {
      mark_frame_used(frame);
      return frame_to_aligned_addr(frame);
    }
  }
  return 0;
}

void pmm_frame_free(paddr_t frame_addr) {
  if (frame_addr == 0 || (frame_addr & (FRAME_SIZE - 1)) != 0)
    return;

  if (!in_range(frame_addr, memory_range))
    return;

  uint64_t frame = aligned_addr_to_frame(frame_addr);
  if (frame >= total_frames)
    return;

  mark_frame_free(frame);
}

void pmm_mark_range_free(paddr_t from, paddr_t to) {
  paddr_t start = align_up(from, FRAME_SIZE);
  paddr_t end = align_down(to, FRAME_SIZE);

  if (start >= end)
    return;

  uint64_t start_frame = (start - memory_range.start) / FRAME_SIZE;
  uint64_t end_frame = (end - memory_range.start) / FRAME_SIZE;

  for (uint64_t f = start_frame; f < end_frame && f < total_frames; f++) {
    if (f == 0)
      continue;

    if (bitmap_test(bitmap, f)) {
      bitmap_clear(bitmap, f);
      frame_refcounts[f] = 0;
      used_frames--;
    }
  }
}

void pmm_init(Range total_range, Range *usable_ranges, int usable_ranges_count,
              Range *critical_ranges, int critical_ranges_count) {
  memory_range = total_range;
  total_frames = (memory_range.end - memory_range.start) / FRAME_SIZE;

  bitmap_size_bytes = (total_frames + 7) / 8;
  memset(bitmap, 0xFF, bitmap_size_bytes);
  used_frames = total_frames;

  for (int i = 0; i < usable_ranges_count; i++) {
    pmm_mark_range_free(usable_ranges[i].start, usable_ranges[i].end);
  }

  for (int i = 0; i < critical_ranges_count; i++) {
    pmm_mark_range_used(critical_ranges[i].start, critical_ranges[i].end);
  }

  initialized = true;
}

uint64_t frame_get_free_count() { return total_frames - used_frames; }
uint64_t frame_get_used_count() { return used_frames; }

// -1 to exclude the reserved physical frame 0
uint64_t frame_get_total_count() { return total_frames - 1; }
