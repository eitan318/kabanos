#include "boot/bootparams.h"
#include "memory_management/early_pmm.h"
#include "memory_management/pmm.h"
#include "memory_management/vmm.h"
#include "memory_management/vmspace.h"
#include "ut/ut_framework.h"

static page_dir_t *g_pd;
static vmspace_t *g_vmspace;

int ut_simple_mapping(void) {
  if (!vm_map(g_pd, 0x400000, 0x200000, PAGE_READWRITE))
    return UT_FAIL;
  return (vm_translate(g_pd, 0x400000) == 0x200000) ? UT_PASS : UT_FAIL;
}

int ut_multiple_mappings(void) {
  uint32_t map[][2] = {{0x400000, 0x100000},
                       {0x401000, 0x101000},
                       {0x800000, 0x200000},
                       {0x801000, 0x201000},
                       {0xC0000000, 0x300000}};
  int n = sizeof(map) / sizeof(map[0]);

  for (int i = 0; i < n; i++)
    if (!vm_map(g_pd, map[i][0], map[i][1], PAGE_READWRITE))
      return UT_FAIL;

  for (int i = 0; i < n; i++)
    if (vm_translate(g_pd, map[i][0]) != map[i][1])
      return UT_FAIL;

  return UT_PASS;
}

int ut_unmapping(void) {
  if (!vm_map(g_pd, 0x400000, 0x200000, PAGE_READWRITE))
    return UT_FAIL;
  if (!vm_unmap(g_pd, 0x400000))
    return UT_FAIL;
  return (vm_translate(g_pd, 0x400000) == 0) ? UT_PASS : UT_FAIL;
}

int ut_identity_mapping(void) {
  uint32_t n = 0x400000 / PAGE_SIZE;

  for (uint32_t i = 0; i < n; i++) {
    uint32_t addr = i * PAGE_SIZE;
    if (!vm_map(g_pd, addr, addr, PAGE_READWRITE))
      return UT_FAIL;
  }

  for (uint32_t i = 0; i < n; i += 100) {
    uint32_t addr = i * PAGE_SIZE;
    if (vm_translate(g_pd, addr) != addr)
      return UT_FAIL;
  }

  return UT_PASS;
}

int ut_page_offset_preservation(void) {
  if (!vm_map(g_pd, 0x400000, 0x200000, PAGE_READWRITE))
    return UT_FAIL;

  uint32_t off[] = {0, 1, 0x100, 0x500, 0xFFF};
  for (int i = 0; i < 5; i++)
    if (vm_translate(g_pd, 0x400000 + off[i]) != 0x200000 + off[i])
      return UT_FAIL;

  return UT_PASS;
}

int ut_unmap_nonexistent(void) {
  vm_unmap(g_pd, 0x400000);
  return (vm_translate(g_pd, 0x400000) == 0) ? UT_PASS : UT_FAIL;
}

int ut_remap_page(void) {
  if (!vm_map(g_pd, 0x400000, 0x200000, PAGE_READWRITE))
    return UT_FAIL;
  if (!vm_map(g_pd, 0x400000, 0x300000, PAGE_READWRITE))
    return UT_FAIL;
  return (vm_translate(g_pd, 0x400000) == 0x300000) ? UT_PASS : UT_FAIL;
}

int ut_different_page_tables(void) {
  if (!vm_map(g_pd, 0x400000, 0x100000, PAGE_READWRITE))
    return UT_FAIL;
  if (!vm_map(g_pd, 0x800000, 0x200000, PAGE_READWRITE))
    return UT_FAIL;

  uint32_t r1 = vm_translate(g_pd, 0x400000);
  uint32_t r2 = vm_translate(g_pd, 0x800000);
  return (r1 == 0x100000 && r2 == 0x200000) ? UT_PASS : UT_FAIL;
}

static int suite_setup() {
  Range mem = {0, 64 * 1024 * 1024};
  kernel_vmspace_create(g_vmspace, mem);
  g_pd = g_vmspace->pd;
  if (!g_pd)
    return UT_FAIL;

  vmspace_switch(g_vmspace);

  Range used[] = {{0, 0x100000}, {0x200000, 0x300000}};
  pmm_init(mem, used, 2);

  return 0;
}

static int suite_teardown() {
  vmspace_destroy(g_vmspace);
  return 0;
}

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

ut_test_suite_t paging_suite = {
    .suite_name = "Paging",
    .suite_setup = suite_setup,
    .suite_teardown = suite_teardown,
    .tests = tests,
    .num_tests = sizeof(tests) / sizeof(tests[0]),
};
