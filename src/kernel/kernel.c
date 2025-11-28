#include "arch/i686/vga_text.h"
#include "boot/bootparams.h"
#include "hal/hal.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "include/string.h"
#include "initrd/initrd.h"
#include "modules/modules.h"
#include "ut/ata/ata_ut_main.h"
#include "ut/frame_allocator/frame_allocator_ut_main.h"
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

  prompt_for_keyboard();
  ut_frame_allocator_main();

  for (;;) {
  }
}
