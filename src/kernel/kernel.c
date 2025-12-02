#include "arch/i686/vga_text.h"
#include "boot/bootparams.h"
#include "hal/hal.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "include/string.h"
#include "initrd/initrd.h"
#include "modules/modules.h"
#include "paging/paging.h"
#include "ut/ata/ata_ut_main.h"
#include "ut/frame_allocator/frame_allocator_ut_main.h"
#include "ut/paging/paging_ut_main.h"
#include "ut/keyboard_driver.h"
#include "ut/printing_info/print_boot_info.h"
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

  // Initialize frame allocator for paging tests
  debugf("\n[Initializing Frame Allocator for Paging Tests]\n");
  FrameAllocator test_allocator;
  frame_allocator_init(&test_allocator, &boot_params.memory_map);
  
  debugf("Frame allocator initialized\n");
  debugf("Total frames: %llu\n", frame_get_total_count(&test_allocator));
  debugf("Free frames: %llu\n", frame_get_free_count(&test_allocator));
  
  debugf("\n*** STARTING PAGING TESTS WITH ACTUAL PAGING ***\n");
  
  // Run paging tests - these will actually enable and test paging!
  paging_tests_run(&test_allocator);
  
  debugf("\n*** PAGING TESTS COMPLETE ***\n");
  debugf("System is back to non-paged mode\n");

  for (int i = 0; i < 1000000000; i++) {}

  prompt_for_keyboard();
  ut_frame_allocator_main();

  for (;;) {
  }
}