#include "arch/i686/vga_text.h"
#include "boot/bootparams.h"
#include "hal/hal.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "initrd/initrd.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"
#include "modules/modules.h"
#include "ut/ata/ata_ut_main.h"
#include "ut/frame_allocator/frame_allocator_ut_main.h"
#include "ut/keyboard_driver.h"
#include "ut/paging/paging_ut_main.h"
#include <stdbool.h>
#include <stdint.h>

extern uint8_t __bss_start;
extern uint8_t __end;

void __attribute__((section(".entry"))) start(BootParams boot_params) {
  memset(&__bss_start, 0, (&__end) - (&__bss_start));
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
  frame_allocator_init(&boot_params.memory_map);

  // Run tests
  ut_paging_main();

  // If you want to keep a page directory after tests, create one
  // But note: tests already enabled paging, so this might conflict
  PageDirectory *pageDir = page_dir_create();

  if (pageDir == NULL) {
    debugf("FAIL: Could not create page directory\n");
    return;
  }

  // Identity map the first 128MB so all frame allocations are accessible
  debugf("Identity mapping kernel memory (0-128MB)...\n");
  for (uint32_t addr = 0; addr < 0x8000000; addr += PAGE_SIZE) {
    if (!paging_page_map(pageDir, addr, addr, PTE_PRESENT | PTE_WRITE)) {
      debugf("FAIL: Could not identity map 0x%x\n", addr);
      return;
    }
  }

  debugf("Enabling paging...\n");
  debugf("Enabling paging...\n");
  paging_enable(pageDir);

  for (int i = 0; i < 1000000000; i++) {
  }

  prompt_for_keyboard();

  for (;;) {
  }
}
