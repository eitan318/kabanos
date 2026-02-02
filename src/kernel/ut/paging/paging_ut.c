#include "arch/types.h"
#include "boot/bootparams.h"
#include "hal.h"
#include "memory_management/pmm.h"
#include "memory_management/vmspace.h"
#include "ut/ut_framework.h"

static arch_vm_t *g_vm;
static vmspace_t *g_test_vmspace;

int ut_simple_mapping(void) {
  if (!hal_vm_map(g_vm, 0x400000, 0x200000, PAGE_READWRITE))
    return UT_FAIL;
  return (hal_vm_virt_to_phys(g_vm, 0x400000) == 0x200000) ? UT_PASS : UT_FAIL;
}

int ut_multiple_mappings(void) {
  uint32_t map[][2] = {
      {0x400000, 0x100000},
      {0x401000, 0x101000},
      {0x800000, 0x200000},
      {0x801000, 0x201000},
  };
  int n = sizeof(map) / sizeof(map[0]);

  for (int i = 0; i < n; i++)
    if (!hal_vm_map(g_vm, map[i][0], map[i][1], PAGE_READWRITE))
      return UT_FAIL;

  for (int i = 0; i < n; i++)
    if (hal_vm_virt_to_phys(g_vm, map[i][0]) != map[i][1])
      return UT_FAIL;

  return UT_PASS;
}

int ut_unmapping(void) {
  if (!hal_vm_map(g_vm, 0x400000, 0x200000, PAGE_READWRITE))
    return UT_FAIL;
  if (!hal_vm_unmap(g_vm, 0x400000))
    return UT_FAIL;
  return (hal_vm_virt_to_phys(g_vm, 0x400000) == 0) ? UT_PASS : UT_FAIL;
}

int ut_identity_mapping(void) {
  uint32_t n = 0x400000 / PAGE_SIZE;

  for (uint32_t i = 0; i < n; i++) {
    uint32_t addr = i * PAGE_SIZE;
    if (!hal_vm_map(g_vm, addr, addr, PAGE_READWRITE))
      return UT_FAIL;
  }

  for (uint32_t i = 0; i < n; i += 100) {
    uint32_t addr = i * PAGE_SIZE;
    if (hal_vm_virt_to_phys(g_vm, addr) != addr)
      return UT_FAIL;
  }

  return UT_PASS;
}

int ut_page_offset_preservation(void) {
  if (!hal_vm_map(g_vm, 0x400000, 0x200000, PAGE_READWRITE))
    return UT_FAIL;

  uint32_t off[] = {0, 1, 0x100, 0x500, 0xFFF};
  for (int i = 0; i < 5; i++)
    if (hal_vm_virt_to_phys(g_vm, 0x400000 + off[i]) != 0x200000 + off[i])
      return UT_FAIL;

  return UT_PASS;
}

int ut_unmap_nonexistent(void) {
  hal_vm_unmap(g_vm, 0x400000);
  return (hal_vm_virt_to_phys(g_vm, 0x400000) == 0) ? UT_PASS : UT_FAIL;
}

int ut_remap_page(void) {
  if (!hal_vm_map(g_vm, 0x400000, 0x200000, PAGE_READWRITE))
    return UT_FAIL;
  if (!hal_vm_map(g_vm, 0x400000, 0x300000, PAGE_READWRITE))
    return UT_FAIL;
  return (hal_vm_virt_to_phys(g_vm, 0x400000) == 0x300000) ? UT_PASS : UT_FAIL;
}

int ut_different_page_tables(void) {
  if (!hal_vm_map(g_vm, 0x400000, 0x100000, PAGE_READWRITE))
    return UT_FAIL;
  if (!hal_vm_map(g_vm, 0x800000, 0x200000, PAGE_READWRITE))
    return UT_FAIL;

  uint32_t r1 = hal_vm_virt_to_phys(g_vm, 0x400000);
  uint32_t r2 = hal_vm_virt_to_phys(g_vm, 0x800000);
  return (r1 == 0x100000 && r2 == 0x200000) ? UT_PASS : UT_FAIL;
}

static int suite_setup() {
  g_test_vmspace = vmspace_create();
  g_vm = g_test_vmspace->arch;
  if (!g_vm)
    return UT_FAIL;

  vmspace_switch(g_test_vmspace);
  return 0;
}

static int suite_teardown() {
  vmspace_destroy(g_test_vmspace);
  extern vmspace_t *g_kernel_vmspace;
  vmspace_switch(g_kernel_vmspace);
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
