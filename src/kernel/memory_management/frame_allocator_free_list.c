#include "frame_allocator.h"
#include <stddef.h>

// Free list implementation structure
FreeFrame *g_free_list_head; // Head of free list
uint64_t g_total_frames;     // Total number of frames
uint64_t g_free_frames;      // Number of free frames

// Add a single frame to the free list
static void add_frame_to_freelist(uint64_t frame_addr) {
  FreeFrame *frame = (FreeFrame *)frame_addr;
  frame->next = g_free_list_head;
  g_free_list_head = frame;
  g_free_frames++;
}

// Remove a frame from the free list if it exists
static bool remove_frame_from_freelist(uint64_t frame_addr) {
  FreeFrame **current = &g_free_list_head;

  while (*current != NULL) {
    if ((uint64_t)*current == frame_addr) {
      *current = (*current)->next;
      g_free_frames--;
      return true;
    }
    current = &(*current)->next;
  }
  return false;
}

// Find highest address in memory map
static uint64_t find_highest_address(MemoryMap *mmap) {
  uint64_t highest = 0;
  for (int i = 0; i < mmap->region_count; i++) {
    uint64_t end = mmap->regions[i].base + mmap->regions[i].length;
    if (end > highest) {
      highest = end;
    }
  }
  return highest;
}

// Add all frames in a range to the free list
static void add_range_to_freelist(uint64_t start_addr, uint64_t end_addr) {
  // Align start up to page boundary
  uint64_t start = (start_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  // Align end down to page boundary
  uint64_t end = end_addr & ~(PAGE_SIZE - 1);

  for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
    add_frame_to_freelist(addr);
  }
}

void frame_allocator_init(MemoryMap *mmap) {
  g_free_list_head = NULL;
  g_free_frames = 0;

  uint64_t highest_addr = find_highest_address(mmap);
  g_total_frames = (highest_addr + PAGE_SIZE - 1) / PAGE_SIZE;

  // Add all usable memory regions to free list
  for (int i = 0; i < mmap->region_count; i++) {
    if (mmap->regions[i].type == E820_USABLE) {
      add_range_to_freelist(mmap->regions[i].base,
                            mmap->regions[i].base + mmap->regions[i].length);
    }
  }
}

uint64_t frame_alloc() {
  if (g_free_list_head == NULL) {
    return 0;
  }

  // Pop from head of free list
  FreeFrame *frame = g_free_list_head;
  g_free_list_head = frame->next;
  g_free_frames--;

  return (uint64_t)frame;
}

void frame_free(uint64_t frame_addr) {
  // Validate frame address
  if (frame_addr == 0 || frame_addr % PAGE_SIZE != 0) {
    return;
  }

  if (frame_addr / PAGE_SIZE >= g_total_frames) {
    return;
  }

  // Check for double free
  FreeFrame *current = g_free_list_head;
  while (current != NULL) {
    if ((uint64_t)current == frame_addr) {
      return; // Double free detected
    }
    current = current->next;
  }

  // Add to free list
  add_frame_to_freelist(frame_addr);
}

void frame_mark_range_used(uint64_t start_addr, uint64_t end_addr) {
  // Align start up to page boundary
  uint64_t start = (start_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  // Align end down to page boundary
  uint64_t end = end_addr & ~(PAGE_SIZE - 1);

  // Remove each frame in the range from free list
  for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
    remove_frame_from_freelist(addr);
  }
}

uint64_t frame_get_free_count() { return g_free_frames; }

uint64_t frame_get_used_count() { return g_total_frames - g_free_frames; }

uint64_t frame_get_total_count() { return g_total_frames; }
