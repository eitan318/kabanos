#include "arch/i686/vga_text.h"
#include "boot/bootparams.h"
#include "hal/hal.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "initrd/initrd.h"
#include "kmalloc.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"
#include "modules/modules.h"
#include "process/pcb.h"
#include "ut/ata/ata_ut_main.h"
#include "ut/frame_allocator/frame_allocator_ut_main.h"
#include "ut/keyboard_driver.h"
#include "ut/paging/paging_ut_main.h"
#include "ut/pcb/pcb_ut_main.h"
#include <stdbool.h>
#include <stdint.h>

extern uint8_t _kernel_start[], _kernel_end[], _bss_start[];

void __attribute__((section(".entry"))) start(BootParams boot_params) {
  memset(_bss_start, 0, _kernel_end - _bss_start);
  vga_clrscr();
  vga_setcursor(0, 0);

  debugf("[Kernel starting...]\n");

  // Initialize initrd
  if (boot_params.initrd_start && boot_params.initrd_size > 0) {
    initrd_init(boot_params.initrd_start, boot_params.initrd_size);
    initrd_list_files();
  } else {
    debugf("No initrd provided\n");
  }

  // Initialize modules
  if (boot_params.module_count > 0) {
    modules_init(&boot_params);
  } else {
    debugf("No modules provided\n");
  }

  hal_init();

  __asm__ volatile("sti");

  // Initialize frame allocator ONCE before tests
  frame_allocator_init(&boot_params);

  PageDirectory *kernel_page_dir = page_dir_create();

  if (kernel_page_dir == NULL) {
    debugf("FAIL: Could not create page directory\n");
    return;
  }

  // Identity map the first 128MB so all frame allocations are accessible
  debugf("Identity mapping kernel memory (0-128MB)...\n");
  for (uint32_t addr = 0; addr < 0x8000000; addr += PAGE_SIZE) {
    if (!paging_page_map(kernel_page_dir, addr, addr,
                         PTE_PRESENT | PTE_WRITE)) {
      debugf("FAIL: Could not identity map 0x%x\n", addr);
      return;
    }
  }

  paging_enable(kernel_page_dir);
  kmalloc_init(kernel_page_dir);

  // Initialize FAT filesystem
  // Assuming first partition starts at LBA 2048 (common for FAT partitions)
  // You'll need to adjust this based on your disk layout
  // Initialize FAT filesystem at the CORRECT partition offset
if (fat_initialize(34, 0)) {  // Partition starts at sector 34!
  debugf("FAT filesystem initialized\n");
  
  // Load calc.elf from root (files are copied to root by your script)
  void *entry_point = load_elf(kernel_page_dir, "/calc.elf");
  
  if (entry_point) {
    debugf("calc.elf loaded successfully at entry point: 0x%x\n", 
           (uint32_t)entry_point);
  } else {
    debugf("Failed to load calc.elf\n");
  }
} else {
  debugf("Failed to initialize FAT filesystem\n");
}

  prompt_for_keyboard();

  for (;;) {
  }
}
