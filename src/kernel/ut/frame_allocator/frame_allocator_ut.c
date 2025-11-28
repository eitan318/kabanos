#include "frame_allocator/frame_allocator.h"
#include "include/stdio.h"
#include "include/string.h"
#include "ut/ut_framework.h"
#include <stdint.h>

MemoryMap create_test_memory_map(void) {
  static MemoryRegion regions[4];

  // Region 0: First 640KB (usable)
  regions[0].base = 0x0;
  regions[0].length = 640 * 1024;
  regions[0].type = E820_USABLE;

  // Region 1: 640KB-1MB (reserved for VGA, BIOS, etc.)
  regions[1].base = 640 * 1024;
  regions[1].length = 384 * 1024;
  regions[1].type = E820_RESERVED;

  // Region 2: 1MB-16MB (usable)
  regions[2].base = 1024 * 1024;
  regions[2].length = 15 * 1024 * 1024;
  regions[2].type = E820_USABLE;

  // Region 3: 16MB-20MB (usable)
  regions[3].base = 16 * 1024 * 1024;
  regions[3].length = 4 * 1024 * 1024;
  regions[3].type = E820_USABLE;

  MemoryMap mmap;
  mmap.regions = regions;
  mmap.region_count = 4;

  return mmap;
}

/*=============================================================================
 * BASIC ALLOCATION TESTS
 *===========================================================================*/

int ut_basic_allocation(void) {
  MemoryMap mmap = create_test_memory_map();
  FrameAllocator allocator;

  frame_allocator_init(&allocator, &mmap);

  debugf("  Total frames: %lu\n", frame_get_total_count(&allocator));
  debugf("  Free frames: %lu\n", frame_get_free_count(&allocator));
  debugf("  Used frames: %lu\n", frame_get_used_count(&allocator));

  if (frame_get_free_count(&allocator) == 0) {
    debugf("FAIL: No free frames available\n");
    return UT_FAIL;
  }

  // Allocate first frame
  uint64_t frame1 = frame_alloc(&allocator);
  debugf("  Allocated frame 1: 0x%lx\n", frame1);

  if (frame1 == 0) {
    debugf("FAIL: Failed to allocate frame1\n");
    return UT_FAIL;
  }
  if (frame1 % PAGE_SIZE != 0) {
    debugf("FAIL: Frame1 not page-aligned\n");
    return UT_FAIL;
  }

  // Allocate second frame
  uint64_t frame2 = frame_alloc(&allocator);
  debugf("  Allocated frame 2: 0x%lx\n", frame2);

  if (frame2 == 0) {
    debugf("FAIL: Failed to allocate frame2\n");
    return UT_FAIL;
  }
  if (frame2 == frame1) {
    debugf("FAIL: Got duplicate frame\n");
    return UT_FAIL;
  }
  if (frame2 % PAGE_SIZE != 0) {
    debugf("FAIL: Frame2 not page-aligned\n");
    return UT_FAIL;
  }

  return UT_PASS;
}

int ut_free_and_realloc(void) {
  MemoryMap mmap = create_test_memory_map();
  FrameAllocator allocator;

  frame_allocator_init(&allocator, &mmap);

  uint64_t initial_free = frame_get_free_count(&allocator);

  // Allocate frame
  uint64_t frame = frame_alloc(&allocator);
  UT_ASSERT_SUCCESS(frame, "Frame allocation");
  debugf("  Allocated: 0x%lx\n", frame);

  uint64_t after_alloc = frame_get_free_count(&allocator);
  if (after_alloc != initial_free - 1) {
    debugf("FAIL: Free count incorrect after alloc: %lu (expected %lu)\n",
           after_alloc, initial_free - 1);
    return UT_FAIL;
  }

  // Free frame
  frame_free(&allocator, frame);
  debugf("  Freed: 0x%lx\n", frame);

  uint64_t after_free = frame_get_free_count(&allocator);
  if (after_free != initial_free) {
    debugf("FAIL: Free count incorrect after free: %lu (expected %lu)\n",
           after_free, initial_free);
    return UT_FAIL;
  }

  // Reallocate
  uint64_t frame2 = frame_alloc(&allocator);
  UT_ASSERT_SUCCESS(frame2, "Frame reallocation");
  debugf("  Reallocated: 0x%lx\n", frame2);

  return UT_PASS;
}

int ut_multiple_alloc_free(void) {
  MemoryMap mmap = create_test_memory_map();
  FrameAllocator allocator;

  frame_allocator_init(&allocator, &mmap);

#define NUM_FRAMES 10
  uint64_t frames[NUM_FRAMES];

  uint64_t initial_free = frame_get_free_count(&allocator);

  // Allocate multiple frames
  debugf("  Allocating %d frames...\n", NUM_FRAMES);
  for (int i = 0; i < NUM_FRAMES; i++) {
    frames[i] = frame_alloc(&allocator);
    if (frames[i] == 0) {
      debugf("FAIL: Failed to allocate frame %d\n", i);
      return UT_FAIL;
    }

    // Check for duplicates
    for (int j = 0; j < i; j++) {
      if (frames[i] == frames[j]) {
        debugf("FAIL: Duplicate frame detected: frames[%d] == frames[%d] = "
               "0x%lx\n",
               i, j, frames[i]);
        return UT_FAIL;
      }
    }
  }

  uint64_t after_alloc = frame_get_free_count(&allocator);
  if (after_alloc != initial_free - NUM_FRAMES) {
    debugf("FAIL: Free count after alloc: %lu (expected %lu)\n", after_alloc,
           initial_free - NUM_FRAMES);
    return UT_FAIL;
  }
  debugf("  Free count: %lu -> %lu\n", initial_free, after_alloc);

  // Free all frames
  debugf("  Freeing %d frames...\n", NUM_FRAMES);
  for (int i = 0; i < NUM_FRAMES; i++) {
    frame_free(&allocator, frames[i]);
  }

  uint64_t after_free = frame_get_free_count(&allocator);
  if (after_free != initial_free) {
    debugf("FAIL: Memory leak detected! Free count: %lu (expected %lu)\n",
           after_free, initial_free);
    return UT_FAIL;
  }
  debugf("  Free count: %lu -> %lu\n", after_alloc, after_free);

  return UT_PASS;
}

/*=============================================================================
 * ERROR HANDLING TESTS
 *===========================================================================*/

int ut_double_free(void) {
  MemoryMap mmap = create_test_memory_map();
  FrameAllocator allocator;

  frame_allocator_init(&allocator, &mmap);

  uint64_t frame = frame_alloc(&allocator);
  UT_ASSERT_SUCCESS(frame, "Frame allocation");

  uint64_t before_free = frame_get_free_count(&allocator);

  // First free
  frame_free(&allocator, frame);
  uint64_t after_first_free = frame_get_free_count(&allocator);
  if (after_first_free != before_free + 1) {
    debugf("FAIL: Free count incorrect after first free: %lu (expected %lu)\n",
           after_first_free, before_free + 1);
    return UT_FAIL;
  }

  // Second free (should be ignored)
  frame_free(&allocator, frame);
  uint64_t after_second_free = frame_get_free_count(&allocator);
  if (after_second_free != after_first_free) {
    debugf("FAIL: Double free not detected! Count: %lu (expected %lu)\n",
           after_second_free, after_first_free);
    return UT_FAIL;
  }

  return UT_PASS;
}

int ut_invalid_operations(void) {
  MemoryMap mmap = create_test_memory_map();
  FrameAllocator allocator;

  frame_allocator_init(&allocator, &mmap);

  uint64_t initial_free = frame_get_free_count(&allocator);

  // Try to free invalid addresses
  frame_free(&allocator, 0);                    // NULL
  frame_free(&allocator, 0x123);                // Unaligned
  frame_free(&allocator, 0xFFFFFFFFFFFF0000UL); // Out of range

  // Free count should not change
  uint64_t after_invalid = frame_get_free_count(&allocator);
  if (after_invalid != initial_free) {
    debugf("FAIL: Invalid operations changed free count: %lu -> %lu\n",
           initial_free, after_invalid);
    return UT_FAIL;
  }

  return UT_PASS;
}

/*=============================================================================
 * ADVANCED TESTS
 *===========================================================================*/

int ut_mark_range_used(void) {
  MemoryMap mmap = create_test_memory_map();
  FrameAllocator allocator;

  frame_allocator_init(&allocator, &mmap);

  uint64_t initial_free = frame_get_free_count(&allocator);

  // Mark a range as used (e.g., kernel memory)
  uint64_t kernel_start = 0x100000; // 1MB
  uint64_t kernel_end = 0x200000;   // 2MB
  uint64_t frames_to_mark = (kernel_end - kernel_start) / PAGE_SIZE;

  debugf("  Marking 0x%lx-0x%lx as used (%lu frames)\n", kernel_start,
         kernel_end, frames_to_mark);

  frame_mark_range_used(&allocator, kernel_start, kernel_end);

  uint64_t after_mark = frame_get_free_count(&allocator);
  debugf("  Free frames: %lu -> %lu\n", initial_free, after_mark);

  if (after_mark > initial_free) {
    debugf("FAIL: Free frames increased after marking used!\n");
    return UT_FAIL;
  }

  return UT_PASS;
}

int ut_exhaustion(void) {
  MemoryMap mmap = create_test_memory_map();
  FrameAllocator allocator;

  frame_allocator_init(&allocator, &mmap);

  uint64_t total_free = frame_get_free_count(&allocator);
  debugf("  Available frames: %lu\n", total_free);

// Allocate all available frames
#define MAX_TEST_FRAMES 5000
  static uint64_t frames[MAX_TEST_FRAMES];
  uint64_t allocated = 0;

  uint64_t frames_to_test =
      total_free < MAX_TEST_FRAMES ? total_free : MAX_TEST_FRAMES;

  for (uint64_t i = 0; i < frames_to_test; i++) {
    frames[i] = frame_alloc(&allocator);
    if (frames[i] != 0) {
      allocated++;
    } else {
      break;
    }
  }

  debugf("  Allocated %lu frames\n", allocated);
  if (allocated != frames_to_test) {
    debugf("FAIL: Expected to allocate %lu frames, got %lu\n", frames_to_test,
           allocated);
    return UT_FAIL;
  }

  // Try to allocate when exhausted (if we allocated all)
  if (frames_to_test == total_free) {
    if (frame_get_free_count(&allocator) != 0) {
      debugf("FAIL: Free count should be 0, got %lu\n",
             frame_get_free_count(&allocator));
      return UT_FAIL;
    }

    uint64_t should_fail = frame_alloc(&allocator);
    if (should_fail != 0) {
      debugf("FAIL: Allocation should fail when exhausted, got 0x%lx\n",
             should_fail);
      return UT_FAIL;
    }
    debugf("  Allocation correctly failed when exhausted\n");
  }

  // Free all
  for (uint64_t i = 0; i < allocated; i++) {
    frame_free(&allocator, frames[i]);
  }

  debugf("  All frames freed successfully\n");

  return UT_PASS;
}

/*=============================================================================
 * DEFINE THE TEST SUITE
 *===========================================================================*/

static ut_test_case_t tests[] = {
    UT_TEST(ut_basic_allocation),    UT_TEST(ut_free_and_realloc),
    UT_TEST(ut_multiple_alloc_free), UT_TEST(ut_double_free),
    UT_TEST(ut_invalid_operations),  UT_TEST(ut_mark_range_used),
    UT_TEST(ut_exhaustion)};

// Export the suite
ut_test_suite_t frame_allocator_suite = {
    .suite_name = "Frame Allocator",
    .setup = NULL,
    .teardown = NULL,
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .tests = tests,
    .num_tests = sizeof(tests) / sizeof(tests[0]),
};
