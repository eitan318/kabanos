#include "arch/i686/vga_text.h"
#include "boot/bootparams.h"
#include "fat/fat.h"
#include "hal/hal.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "initrd/initrd.h"
#include "kernel_boot_info.h"
#include "memory_management/early_pmm.h"
#include "memory_management/kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/pmm.h"
#include "memory_management/vmm.h"
#include "memory_management/vmspace.h"
#include "memory_map.h"
#include "modules/modules.h"
#include "process/schedualer.h"
#include "ut/ata/ata_ut_main.h"
#include "ut/keyboard_driver.h"
#include "ut/paging/paging_ut_main.h"
#include "utils/range.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

vmspace_t *g_kernel_vmspace;
Range g_kernel_virt_range;
Range g_kernel_phys_range;

extern uint8_t _bss_start[], _bss_end[];
void kmain(uint32_t mb2_ptr) {
  extern uint8_t _kernel_start[], _kernel_end[];
  g_kernel_virt_range.start = (uintptr_t)&_kernel_start;
  g_kernel_virt_range.end = (uintptr_t)&_kernel_end;
  g_kernel_phys_range.start = g_kernel_virt_range.start - KERNEL_BASE;
  g_kernel_phys_range.end = g_kernel_virt_range.end - KERNEL_BASE;

  early_pmm_init(g_kernel_phys_range.end);
  KernelBootInfo *kernel_boot_info =
      parse_multiboot2_early((mb2_info_t *)mb2_ptr);
  mb2_ptr = 0; // disabling use of unparsed, low half params

  uintptr_t bss_size = (uintptr_t)(_bss_end - _bss_start);
  // memset(&_bss_start[0], 0, bss_size);
  debugf("[Kernel starting...]\n");

  Range total_memory_range = get_memory_range(&kernel_boot_info->memory_map);
  size_t count;
  Range *unusable_memory_ranges =
      get_unusable_memory_ranges(kernel_boot_info, total_memory_range, &count);

  // From now on, no early pmm
  early_pmm_disable();
  pmm_init(total_memory_range, unusable_memory_ranges, count);

  static vmspace_t kernel_vmspace = {0};
  kernel_vmspace_create(&kernel_vmspace);
  g_kernel_vmspace = &kernel_vmspace;

  if (g_kernel_vmspace == NULL) {
    debugf("FAIL: Could not create page directory\n");
    return;
  }

  // From now on no lower half mapping
  vmspace_switch(g_kernel_vmspace);

  hal_init();

  asm volatile("sti");

  kmalloc_init();

  if (!fat_initialize(34, 0)) {
    debugf("Failed to initialize FAT\n");
    for (;;) {
    }
  }

  debugf("Testing tasks\n");
  test_tasks();

  for (;;) {
  }
}
