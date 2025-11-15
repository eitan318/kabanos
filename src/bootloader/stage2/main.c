#include "cmdline.h"
#include "cpu_info.h"
#include "disk.h"
#include "fat.h"
#include "gdt.h"
#include "kernel_sectors.h"
#include "mbr.h"
#include "memory_map.h"
#include "stage2_sectors.h"
#include "stdio.h"
#include "vga_text.h"
#include <stdint.h>

#define KERNEL_FINAL_ADDR 0x100000

typedef struct {
  const MBRPartitionEntry *partition_table;
  int partitions_count;
  DiskParams disk_params;
  const E820Entry *memory_map;
  int memory_map_entry_count;
  CPUInfo *cpu_info;
  char *cmdline_buffer;
  int cmdline_size;
} BootInfo;

void halt() {
  debugf("\nSystem halted.\n");
  while (1) {
    __asm__ volatile("cli; hlt");
  }
}

#define CMDLINE_SIZE 2048

typedef void (*KernelStart)(BootInfo *bootInfo);

void __attribute__((cdecl)) start(uint32_t boot_drive) {
  i686_gdt_init();

  vga_setcursor(0, 0);
  debugf("Stage2: Initializing...\n");

  // Initialize disk
  DiskParams disk_params = {0};
  if (!disk_init(boot_drive, &disk_params)) {
    debugf("ERROR: Failed to initialize disk!\n");
    halt();
  }

  // Read MBR
  mbr_read(&disk_params);
  const MBRPartitionEntry *partition_table = mbr_get_partitions();
  int partitions_count = mbr_get_partition_count();

  debugf("\nInitializing FAT filesystem from partition 1...\n");
  if (!fat_initialize(&disk_params, partition_table[0].lba_start)) {
    debugf("ERROR: Failed to initialize FAT filesystem!\n");
    halt();
  }

  if (!memory_map_init()) {
    debugf("ERROR: Failed to init memory map\n");
    halt();
  }
  const E820Entry *memory_map = memory_map_get();
  int memory_map_count = memory_map_count_get();

  // Collect CPU info
  CPUInfo cpu_info;
  collect_cpu_info(&cpu_info);

  // Read command line
  char config_buffer[2048];
  int config_size = fat_read_file("/boot.cfg", config_buffer);
  if (config_size < 0)
    config_size = 0; // fallback

  BCD bcd;
  bcd_parse_into(config_buffer, &bcd);
  char cmdline[CMDLINE_SIZE];
  bcd_cmdline_construct(&bcd, cmdline);

  // Fill BootInfo
  BootInfo bootInfo = {0};
  bootInfo.cpu_info = &cpu_info;
  bootInfo.disk_params = disk_params;
  bootInfo.cmdline_buffer = cmdline;
  bootInfo.cmdline_size = CMDLINE_SIZE;
  bootInfo.memory_map = memory_map;
  bootInfo.memory_map_entry_count = memory_map_count;
  bootInfo.partition_table = partition_table;
  bootInfo.partitions_count = partitions_count;

  int kernel_size = fat_read_file(bcd.kernel, (void *)KERNEL_FINAL_ADDR);
  if (kernel_size < 0) {
    debugf("ERROR: Failed to load kernel (error code: %d)!\n", kernel_size);
    halt();
  }

  debugf("Kernel loaded successfully, jumping...\n");
  KernelStart kernelStart = (KernelStart)KERNEL_FINAL_ADDR;
  kernelStart(&bootInfo);

  debugf("ERROR: Kernel returned!\n");
}
