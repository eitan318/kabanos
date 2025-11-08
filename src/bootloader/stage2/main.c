// start.c - Stage2 C entry point
#include "disk.h"
#include "fat.h"
#include "kernel_sectors.h"
#include "memory.h"
#include "stage2_sectors.h"
#include "stdio.h"
#include "vga_text.h"
#include <stdint.h>

#define KERNEL_TEMP_ADDR 0x10000
#define KERNEL_FINEL_ADDR 0x100000

uint8_t *Kernel = (uint8_t *)KERNEL_FINEL_ADDR;
typedef void (*KernelStart)();

void __attribute__((cdecl)) start(uint32_t boot_drive) {
  debugf("Stage2: Initializing...\n");
  // vga_clrscr();
  vga_setcursor(0, 0);

  debugf("Stage2: Initializing...\n");
  debugf("Boot drive: %x\n", boot_drive);

  // Initialize disk
  DiskParams disk_params = {0};
  if (!disk_init(boot_drive, &disk_params)) {
    debugf("ERROR: Failed to initialize disk!\n");
    goto halt;
  }

  check(&disk_params);
  //
  // // Calculate kernel LBA
  // uint32_t kernel_lba = 1 + STAGE2_SECTORS_TOTAL;
  // debugf("Loading %u sectors of kernel from LBA %u to addr%x\n",
  //        KERNEL_SECTORS_TOTAL, kernel_lba, KERNEL_TEMP_ADDR);
  //
  // // Load kernel
  // if (!disk_read_sectors(&disk_params, kernel_lba, KERNEL_SECTORS_TOTAL,
  //                        (void *)KERNEL_TEMP_ADDR)) {
  //   printf("ERROR: Failed to load kernel!\n");
  //   goto halt;
  // }
  // debugf("Copying kernel from temp addr%x to addr %x\n", KERNEL_TEMP_ADDR,
  //        KERNEL_FINEL_ADDR);
  //
  // uint8_t *kernel_src = (uint8_t *)KERNEL_TEMP_ADDR;
  // uint8_t *kernel_dst = (uint8_t *)KERNEL_FINEL_ADDR;
  //
  // memcpy(kernel_dst, kernel_src, KERNEL_SECTORS_TOTAL * 512);
  //
  // debugf("Kernel loaded successfully, jumping...");
  //
  // // execute kernel
  // KernelStart kernelStart = (KernelStart)Kernel;
  // kernelStart();
  //
  // // Should never reach here
  // debugf("ERROR: Kernel returned!\n");
  //
halt:
  debugf("\nSystem halted.\n");
  while (1) {
    __asm__ volatile("cli; hlt");
  }
}
