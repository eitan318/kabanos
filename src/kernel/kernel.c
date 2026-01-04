#include "arch/i686/vga_text.h"
#include "boot/bootparams.h"
#include "fat/fat.h"
#include "hal/hal.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "initrd/initrd.h"
#include "kmalloc.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"
#include "modules/modules.h"
#include "process/pcb.h"
#include "process/process.h"
#include "process/schedualer.h"
#include "process/task.h"
#include "ut/ata/ata_ut_main.h"
#include "ut/frame_allocator/frame_allocator_ut_main.h"
#include "ut/keyboard_driver.h"
#include "ut/paging/paging_ut_main.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

PageDirectory *g_kernel_page_dir;

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

  g_kernel_page_dir = paging_create_kernel();
  if (g_kernel_page_dir == NULL) {
    debugf("FAIL: Could not create page directory\n");
    return;
  }

  paging_enable(g_kernel_page_dir);

  kmalloc_init(g_kernel_page_dir);

  // Initialize FAT filesystem
  debugf("Initializing FAT filesystem...\n");
  if (!fat_initialize(34, 0)) {
    debugf("Failed to initialize FAT\n");
    for (;;) {
    }
  }

  debugf("FAT initialized\n");

  process_init();

  test_tasks();
  for (;;) {
  }
}

void test_proccess(PageDirectory *kernel_page_dir) {

  Pcb *calc_process = process_create("/calc.elf");

  Pcb kernel_process = {
      .page_directory = (uint32_t *)kernel_page_dir,
      .cpu_context = {0}, // Initialize to zero
      .state = PROCESS_STATE_RUNNING,
  };

  // Manually set the return point
  kernel_process.cpu_context.eip = (uint32_t) && return_here;
  kernel_process.cpu_context.cr3 = (uint32_t)kernel_page_dir;

  // Save current stack state
  __asm__ volatile("mov %%esp, %0\n"
                   "mov %%ebp, %1\n"
                   : "=r"(kernel_process.cpu_context.esp),
                     "=r"(kernel_process.cpu_context.ebp));

  // prompt_for_keyboard();

  process_context_switch(&kernel_process, calc_process);

return_here:
  debugf("KERNEL SUCCESSFULLY RETURNED!");
}
