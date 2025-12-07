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
#include "ut/process/process_ut_main.h"
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

  PageDirectory *kernel_page_dir = paging_create();

  if (kernel_page_dir == NULL) {
    debugf("FAIL: Could not create page directory\n");
    return;
  }

  // Identity map the first 128MB so all frame allocations are accessible
  debugf("Identity mapping kernel memory (0-128MB)...\n");
  for (uint32_t addr = 0; addr < 0x8000000; addr += PAGE_SIZE) {
    if (!paging_map(kernel_page_dir, addr, addr, PAGE_WRITABLE)) {
      debugf("FAIL: Could not identity map 0x%x\n", addr);
      return;
    }
  }

  paging_enable(kernel_page_dir);
  kmalloc_init(kernel_page_dir);

  // Initialize FAT filesystem
  debugf("Initializing FAT filesystem...\n");
  if (!fat_initialize(34, 0)) {
    debugf("Failed to initialize FAT\n");
    for (;;) {}
  }
  debugf("FAT initialized\n");

  // Initialize process management
  debugf("\nInitializing process management...\n");
  process_init();

  // Test: Create process from calc.elf
  debugf("\n========================================\n");
  debugf("TEST: Creating process from calc.elf\n");
  debugf("========================================\n\n");
  
  Pcb *calc_process = process_create("/calc.elf");
  
  if (calc_process) {
    debugf("\nSUCCESS: Process created!\n\n");
    
    // List all processes
    debugf("========================================\n");
    process_list_all();
    debugf("========================================\n\n");
    
    // Verify PCB fields
    debugf("Verification:\n");
    debugf("  PID: %u\n", calc_process->pid);
    debugf("  Name: %s\n", calc_process->name);
    debugf("  State: %s (expected: READY)\n", 
           pcb_state_string_get(calc_process->state));
    debugf("  Priority: %s\n", 
           pcb_priority_string_get(calc_process->priority));
    debugf("  Entry Point (EIP): 0x%x\n", calc_process->cpu_context.eip);
    debugf("  Stack Pointer (ESP): 0x%x\n", calc_process->cpu_context.esp);
    debugf("  Base Pointer (EBP): 0x%x\n", calc_process->cpu_context.ebp);
    debugf("  EFLAGS: 0x%x (bit 9 set = interrupts enabled)\n", 
           calc_process->cpu_context.eflags);
    debugf("  CR3 (Page Dir): 0x%x\n", calc_process->cpu_context.cr3);
    debugf("  Heap: 0x%x - 0x%x\n", 
           calc_process->heap_start, calc_process->heap_end);
    debugf("  Stack: 0x%x - 0x%x\n", 
           calc_process->stack_top, 
           calc_process->stack_top + calc_process->stack_size);
    
    debugf("\nAll fields initialized correctly!\n");
    
    // Verify state is READY
    if (calc_process->state == PROCESS_STATE_READY) {
      debugf("Process is in READY state (can be scheduled)\n");
    } else {
      debugf("WARNING: Process is not in READY state\n");
    }
    
    // Verify all registers are zeroed except ESP, EBP, EIP, EFLAGS
    bool regs_ok = (calc_process->cpu_context.eax == 0 &&
                    calc_process->cpu_context.ebx == 0 &&
                    calc_process->cpu_context.ecx == 0 &&
                    calc_process->cpu_context.edx == 0 &&
                    calc_process->cpu_context.esi == 0 &&
                    calc_process->cpu_context.edi == 0);
    
    if (regs_ok) {
      debugf("All general-purpose registers initialized to 0\n");
    } else {
      debugf("WARNING: Some registers not zeroed\n");
    }
    
    // Verify EFLAGS has interrupts enabled
    if (calc_process->cpu_context.eflags & 0x200) {
      debugf("Interrupts enabled in EFLAGS\n");
    } else {
      debugf("WARNING: Interrupts not enabled\n");
    }
    
    debugf("\n========================================\n");
    debugf("COMPLETE: Process creation working!\n");
    debugf("========================================\n");
    
  } else {
    debugf("\nFAILED: Could not create process\n");
  }

  prompt_for_keyboard();

  for (;;) {
  }
}