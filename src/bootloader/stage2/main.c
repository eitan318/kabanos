// start.c - Stage2 C entry point
#include "arch/i686/disk.h"
#include "arch/i686/gdt.h"
#include "arch/i686/memory_map.h"
#include "arch/i686/vga_text.h"
#include "fat.h"
#include "kernel_sectors.h"
#include "mbr.h"
#include "stage2_sectors.h"
#include "stdio.h"
#include <stdint.h>

#define KERNEL_FINEL_ADDR 0x100000

uint8_t *Kernel = (uint8_t *)KERNEL_FINEL_ADDR;
typedef void (*KernelStart)(const MBRPartitionEntry *partition_table,
                            int num_partitions, DiskParams disk_params,
                            const E820Entry *memory_map,
                            const int memory_map_entry_count);

void __attribute__((cdecl)) start(uint32_t boot_drive) {
  i686_gdt_init();

  vga_setcursor(0, 0);
  debugf("Stage2: Initializing...\n");
  debugf("Boot drive: %x\n", boot_drive);

  // Initialize disk
  DiskParams disk_params = {0};
  if (!disk_init(boot_drive, &disk_params)) {
    debugf("ERROR: Failed to initialize disk!\n");
    goto halt;
  }

  // Read MBR
  mbr_read(&disk_params);
  const MBRPartitionEntry *partition_table = mbr_get_partitions();
  const int partitions_count = mbr_get_partition_count();

  debugf("\nInitializing FAT filesystem from partition 1...\n");
  if (!fat_initialize(&disk_params, partition_table[0].lba_start)) {
    debugf("ERROR: Failed to initialize FAT filesystem!\n");
    goto halt;
  }

  int kernel_size = fat_read_file("/kernel.bin", (void *)KERNEL_FINEL_ADDR);

  if (kernel_size < 0) {
    debugf("ERROR: Failed to load kernel (error code: %d)!\n", kernel_size);
    goto halt;
  }

  if (!memory_map_init()) {
    debugf("ERROR: Failed to init memory map\n", kernel_size);
    goto halt;
  }
  const E820Entry *memory_map = memory_map_get();
  const int memory_map_count = memory_map_count_get();

  debugf("Kernel loaded successfully, jumping...\n");
  KernelStart kernelStart = (KernelStart)KERNEL_FINEL_ADDR;
  kernelStart(partition_table, partitions_count, disk_params, memory_map,
              memory_map_count);

  debugf("ERROR: Kernel returned!\n");

halt:
  debugf("\nSystem halted.\n");
  while (1) {
    __asm__ volatile("cli; hlt");
  }
}
