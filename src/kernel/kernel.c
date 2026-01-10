#include "arch/i686/vga_text.h"
#include "boot/bootparams.h"
#include "fat/fat.h"
#include "hal/hal.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "initrd/initrd.h"
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

  mb2_info_t *mbi = (mb2_info_t *)mb2_ptr;
  early_mem_init();
  KernelBootInfo kernel_boot_info = parse_multiboot2_early(mbi);
  mbi = NULL; // From now, mb2 struct should have been copied and shall not be
              // used

  uintptr_t bss_size = (uintptr_t)(_bss_end - _bss_start);
  // memset(&_bss_start[0], 0, bss_size);

  debugf("[Kernel starting...]\n");

  // Initializing early pmm. from now on, early_pmm is unusable.
  Range total_memory_range = get_memory_range(&kernel_boot_info.memory_map);
  size_t count;
  Range *unusable_memory_ranges =
      get_unusable_memory_ranges(&kernel_boot_info, total_memory_range, &count);
  pmm_init(total_memory_range, unusable_memory_ranges, count);

  vmspace_t kernel_final_vmspace = {0};
  kernel_vmspace_creat(&kernel_final_vmspace);
  g_kernel_vmspace = &kernel_final_vmspace;
  if (g_kernel_vmspace == NULL) {
    debugf("FAIL: Could not create page directory\n");
    return;
  }

  vmspace_switch(g_kernel_vmspace);

  vm_map(g_kernel_vmspace->pd, VGA_SCREEN_BUF, VGA_SCREEN_BUF_PHYS,
         PAGE_READWRITE);

  hal_init();
  vga_clrscr();
  vga_setcursor(0, 0);

  // ut_paging_main(boot_params);

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
