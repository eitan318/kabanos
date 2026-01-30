#include "boot/bootparams.h"
#include "drivers/vga_text.h"
#include "fat/fat.h"
#include "hal.h"
#include "kernel_boot_info.h"
#include "memory_management/kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/memory_map.h"
#include "memory_management/pmm.h"
#include "memory_management/vmspace.h"
#include "proc/exec.h"
#include "sched/sched.h"
#include "stdio.h"
#include "string.h"
#include "ut/ata/ata_ut_main.h"
#include "ut/frame_allocator/frame_allocator_ut_main.h"
#include "ut/keyboard_driver.h"
#include "utils/range.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

vmspace_t *g_kernel_vmspace;
Range g_kernel_virt_range;
Range g_kernel_phys_range;

void kmain(uint32_t mb2_ptr) {
  debugf("[Kernel starting...]\n");
  extern uint8_t _kernel_start[], _kernel_end[];
  g_kernel_virt_range.start = (uintptr_t)&_kernel_start;
  g_kernel_virt_range.end = (uintptr_t)&_kernel_end;
  g_kernel_phys_range.start = g_kernel_virt_range.start - KERNEL_BASE;
  g_kernel_phys_range.end = g_kernel_virt_range.end - KERNEL_BASE;

  KernelBootInfo *kernel_boot_info =
      parse_multiboot2_early((mb2_info_t *)mb2_ptr);
  mb2_ptr = 0; // disabling use of unparsed, low half params

  Range total_memory_range = get_memory_range(&kernel_boot_info->memory_map);
  size_t count;
  Range *unusable_memory_ranges =
      get_unusable_memory_ranges(kernel_boot_info, total_memory_range, &count);

  // From now on, no early pmm
  pmm_init(total_memory_range, unusable_memory_ranges, count);

  static vmspace_t kernel_vmspace = {0};
  kernel_vmspace_create(&kernel_vmspace, total_memory_range);
  g_kernel_vmspace = &kernel_vmspace;

  if (g_kernel_vmspace == NULL) {
    debugf("FAIL: Could not create page directory\n");
    return;
  }

  // From now on no lower half mapping
  vmspace_switch(g_kernel_vmspace);

  // Init hardware
  hal_arch_init();
  vga_clrscr();
  vga_setcursor(0, 0);
  kbd_init();

  hal_interrupts_enable();
  kmalloc_init();

  if (!fat_initialize(34)) {
    debugf("Failed to initialize FAT\n");
    for (;;) {
    }
  }

  debugf("Testing Proc\n");
  sched_init();
  if (process_exec("test_a.elf") != 0)
    debugf("err\n\n");

  if (process_exec("test_b.elf") != 0)
    debugf("err\n\n");

  if (process_exec("test_c.elf") != 0)
    debugf("err\n\n");

  for (;;) {
  }
}
