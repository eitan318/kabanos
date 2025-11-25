#include "arch/i686/vga_text.h"
#include "boot/bootparams.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "include/string.h"
#include "initrd.h"
#include "modules.h"
#include "print_boot_info.h"
#include "hal/hal.h"
#include "keyboard_driver/keyboard_driver.h"
#include <stdbool.h>
#include <stdint.h>

extern uint8_t __bss_start;
extern uint8_t __end;

void __attribute__((section(".entry"))) start(BootParams boot_params) {
  memset(&__bss_start, 0, (&__end) - (&__bss_start));
  vga_clrscr();
  vga_setcursor(0, 0);

  // print_partition_table(boot_params.partition_table);
  // print_memory_map(boot_params.memory_map);
  // print_disk_params(&boot_params.disk_params);
  // print_cpu_info(boot_params.cpu_info);
  // print_cmdline(boot_params.cmdline_buffer, boot_params.cmdline_size);
  //
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

  // Continue with kernel initialization...
  debugf("[Kernel initialization complete]\n");

  hal_init();

  __asm__ volatile("sti");
    
  printf("Keyboard ready - start typing:\n");

  for (;;) {
	  char c = kbd_char_get();
	  if (c != 0) {
		  printf("%c", c);
	  }
  }

  for (;;) {
  }
}
