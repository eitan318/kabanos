#include "boot/bootparams.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"
#include "ut/ut_framework.h"
#include <stdint.h>
#include <string.h>

static PageDirectory *g_test_page_dir;

/*=============================================================================
 * TEST HELPER FUNCTIONS
 *===========================================================================*/

// Create a test memory map for the frame allocator
static MemoryMap create_test_memory_map(void) {
  MemoryMap mmap;
  mmap.region_count = 2;

  // First region: conventional memory from 1MB to 16MB
  mmap.regions[0].base = 0x100000;
  mmap.regions[0].length = 0xF00000; // 15MB
  mmap.regions[0].type = E820_USABLE;

  // Second region: more memory from 32MB to 64MB
  mmap.regions[1].base = 0x2000000;
  mmap.regions[1].length = 0x2000000; // 32MB
  mmap.regions[1].type = E820_USABLE;

  return mmap;
}

/*=============================================================================
 * TEST CASES
 *===========================================================================*/
int ut_simple_mapping(void) {
  // Map virtual address 0x400000 to physical address 0x200000
  uint32_t virt_addr = 0x400000;
  uint32_t phys_addr = 0x200000;

  debugf("  Mapping virt 0x%x to phys 0x%x...\n", virt_addr, phys_addr);
  bool result =
      paging_map(g_test_page_dir, virt_addr, phys_addr, PAGE_WRITABLE);

  if (!result) {
    debugf("FAIL: paging_page_map() returned false\n");
    return UT_FAIL;
  }

  debugf("  Verifying mapping...\n");
  uint32_t retrieved = paging_get_physical(g_test_page_dir, virt_addr);

  if (retrieved != phys_addr) {
    debugf("FAIL: Expected phys 0x%x, got 0x%x\n", phys_addr, retrieved);
    return UT_FAIL;
  }

  return UT_PASS;
}

int ut_multiple_mappings(void) {
  // Map multiple pages
  uint32_t mappings[][2] = {
      {0x400000, 0x100000}, {0x401000, 0x101000},   {0x800000, 0x200000},
      {0x801000, 0x201000}, {0xC0000000, 0x300000},
  };

  int num_mappings = sizeof(mappings) / sizeof(mappings[0]);

  debugf("  Creating %d mappings...\n", num_mappings);
  for (int i = 0; i < num_mappings; i++) {
    uint32_t virt = mappings[i][0];
    uint32_t phys = mappings[i][1];

    if (!paging_map(g_test_page_dir, virt, phys, PAGE_WRITABLE)) {
      debugf("FAIL: Could not map virt 0x%x to phys 0x%x\n", virt, phys);
      return UT_FAIL;
    }
  }

  debugf("  Verifying all mappings...\n");
  for (int i = 0; i < num_mappings; i++) {
    uint32_t virt = mappings[i][0];
    uint32_t expected_phys = mappings[i][1];
    uint32_t retrieved = paging_get_physical(g_test_page_dir, virt);

    if (retrieved != expected_phys) {
      debugf("FAIL: Mapping %d: expected 0x%x, got 0x%x\n", i, expected_phys,
             retrieved);
      return UT_FAIL;
    }
  }

  debugf("  All mappings verified successfully\n");
  return UT_PASS;
}

int ut_unmapping(void) {
  uint32_t virt_addr = 0x400000;
  uint32_t phys_addr = 0x200000;

  debugf("  Mapping virt 0x%x to phys 0x%x...\n", virt_addr, phys_addr);
  if (!paging_map(g_test_page_dir, virt_addr, phys_addr, PAGE_WRITABLE)) {
    debugf("FAIL: Could not create mapping\n");
    return UT_FAIL;
  }

  debugf("  Unmapping virt 0x%x...\n", virt_addr);
  if (!paging_unmap(g_test_page_dir, virt_addr)) {
    debugf("FAIL: paging_page_unmap() returned false\n");
    return UT_FAIL;
  }

  debugf("  Verifying page is unmapped...\n");
  uint32_t retrieved = paging_get_physical(g_test_page_dir, virt_addr);

  if (retrieved != 0) {
    debugf("FAIL: Expected 0 (unmapped), got 0x%x\n", retrieved);
    return UT_FAIL;
  }
  return UT_PASS;
}

int ut_identity_mapping(void) {
  // Identity map first 4MB (0x0 - 0x400000)
  debugf("  Identity mapping first 4MB...\n");
  uint32_t num_pages = 0x400000 / PAGE_SIZE; // 1024 pages

  for (uint32_t i = 0; i < num_pages; i++) {
    uint32_t addr = i * PAGE_SIZE;
    if (!paging_map(g_test_page_dir, addr, addr, PAGE_WRITABLE)) {
      debugf("FAIL: Could not map page at 0x%x\n", addr);
      return UT_FAIL;
    }
  }

  debugf("  Verifying identity mappings...\n");
  // Sample verification (checking every 100th page to avoid spam)
  for (uint32_t i = 0; i < num_pages; i += 100) {
    uint32_t addr = i * PAGE_SIZE;
    uint32_t retrieved = paging_get_physical(g_test_page_dir, addr);

    if (retrieved != addr) {
      debugf("FAIL: Expected identity mapping at 0x%x, got 0x%x\n", addr,
             retrieved);
      return UT_FAIL;
    }
  }

  debugf("  Identity mapping verified successfully\n");
  return UT_PASS;
}

int ut_page_offset_preservation(void) {
  // Map a page
  uint32_t virt_base = 0x400000;
  uint32_t phys_base = 0x200000;

  debugf("  Mapping page at virt 0x%x...\n", virt_base);
  if (!paging_map(g_test_page_dir, virt_base, phys_base, PAGE_WRITABLE)) {
    debugf("FAIL: Could not create mapping\n");
    return UT_FAIL;
  }

  // Test various offsets within the page
  uint32_t offsets[] = {0, 1, 0x100, 0x500, 0xFFF};
  int num_offsets = sizeof(offsets) / sizeof(offsets[0]);

  debugf("  Testing address translation with offsets...\n");
  for (int i = 0; i < num_offsets; i++) {
    uint32_t offset = offsets[i];
    uint32_t virt_addr = virt_base + offset;
    uint32_t expected_phys = phys_base + offset;
    uint32_t retrieved = paging_get_physical(g_test_page_dir, virt_addr);

    if (retrieved != expected_phys) {
      debugf("FAIL: Offset 0x%x: expected 0x%x, got 0x%x\n", offset,
             expected_phys, retrieved);
      return UT_FAIL;
    }
  }

  return UT_PASS;
}

int ut_unmap_nonexistent(void) {
  uint32_t virt_addr = 0x400000;

  debugf("  Attempting to unmap non-existent mapping...\n");
  bool result = paging_unmap(g_test_page_dir, virt_addr);

  // The behavior here depends on implementation - it might return false
  // or handle gracefully. Document what we expect.
  debugf("  Unmap of non-existent page returned: %s\n",
         result ? "true" : "false");

  // Verify nothing broke
  uint32_t retrieved = paging_get_physical(g_test_page_dir, virt_addr);
  if (retrieved != 0) {
    debugf("FAIL: Non-existent page should return 0, got 0x%x\n", retrieved);
    return UT_FAIL;
  }

  return UT_PASS;
}

int ut_remap_page(void) {
  uint32_t virt_addr = 0x400000;
  uint32_t phys_addr1 = 0x200000;
  uint32_t phys_addr2 = 0x300000;

  debugf("  Initial mapping: virt 0x%x -> phys 0x%x...\n", virt_addr,
         phys_addr1);
  if (!paging_map(g_test_page_dir, virt_addr, phys_addr1, PAGE_WRITABLE)) {
    debugf("FAIL: Could not create initial mapping\n");
    return UT_FAIL;
  }

  debugf("  Remapping: virt 0x%x -> phys 0x%x...\n", virt_addr, phys_addr2);
  if (!paging_map(g_test_page_dir, virt_addr, phys_addr2, PAGE_WRITABLE)) {
    debugf("FAIL: Could not remap page\n");
    return UT_FAIL;
  }

  debugf("  Verifying new mapping...\n");
  uint32_t retrieved = paging_get_physical(g_test_page_dir, virt_addr);

  if (retrieved != phys_addr2) {
    debugf("FAIL: Expected 0x%x, got 0x%x\n", phys_addr2, retrieved);
    return UT_FAIL;
  }

  return UT_PASS;
}

int ut_different_page_tables(void) {
  // Map addresses that will be in different page tables
  // Page tables cover 4MB each, so use addresses 0x400000 and 0x800000
  uint32_t virt1 = 0x400000; // Page table 1
  uint32_t virt2 = 0x800000; // Page table 2
  uint32_t phys1 = 0x100000;
  uint32_t phys2 = 0x200000;

  debugf("  Mapping addresses in different page tables...\n");
  if (!paging_map(g_test_page_dir, virt1, phys1, PAGE_WRITABLE)) {
    debugf("FAIL: Could not map first address\n");
    return UT_FAIL;
  }

  if (!paging_map(g_test_page_dir, virt2, phys2, PAGE_WRITABLE)) {
    debugf("FAIL: Could not map second address\n");
    return UT_FAIL;
  }

  debugf("  Verifying both mappings...\n");
  uint32_t retrieved1 = paging_get_physical(g_test_page_dir, virt1);
  uint32_t retrieved2 = paging_get_physical(g_test_page_dir, virt2);

  if (retrieved1 != phys1 || retrieved2 != phys2) {
    debugf("FAIL: Mappings incorrect. Got 0x%x and 0x%x\n", retrieved1,
           retrieved2);
    return UT_FAIL;
  }

  return UT_PASS;
}

/*=============================================================================
 * SETUP AND TEARDOWN
 *===========================================================================*/

int suite_setup() {
  g_test_page_dir = paging_create();
  if (g_test_page_dir == NULL) {
    debugf("FAIL: Could not create page directory\n");
    return UT_FAIL;
  }

  // Identity map the first 128MB so all frame allocations are accessible
  debugf("Identity mapping kernel memory (0-128MB)...\n");
  for (uint32_t addr = 0; addr < 0x8000000; addr += PAGE_SIZE) {
    if (!paging_map(g_test_page_dir, addr, addr, PAGE_WRITABLE)) {
      debugf("FAIL: Could not identity map 0x%x\n", addr);
      return UT_FAIL;
    }
  }

  debugf("Enabling paging...\n");
  paging_enable(g_test_page_dir);

  return 0;
}

int suite_teardown() {
  debugf("Tearing down paging test suite...\n");
  paging_disable();
  paging_destroy(g_test_page_dir);
  return 0;
}

/*=============================================================================
 * DEFINE THE TEST SUITE
 *===========================================================================*/

static ut_test_case_t tests[] = {
    UT_TEST(ut_simple_mapping),
    UT_TEST(ut_multiple_mappings),
    UT_TEST(ut_unmapping),
    UT_TEST(ut_identity_mapping),
    UT_TEST(ut_page_offset_preservation),
    UT_TEST(ut_unmap_nonexistent),
    UT_TEST(ut_remap_page),
    UT_TEST(ut_different_page_tables),
};

// Export the suite
ut_test_suite_t paging_suite = {
    .suite_name = "Paging",
    .setup = NULL,
    .teardown = NULL,
    .suite_setup = suite_setup,
    .suite_teardown = suite_teardown,
    .tests = tests,
    .num_tests = sizeof(tests) / sizeof(tests[0]),
};
