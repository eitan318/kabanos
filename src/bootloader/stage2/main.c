#include "boot/bootparams.h"
#include "cmdline.h"
#include "cpu_info.h"
#include "disk.h"
#include "fat.h"
#include "gdt.h"
#include "mbr.h"
#include "memory_map.h"
#include "stdio.h"
#include "string.h"
#include "vga_text.h"
#include <stdint.h>

#define KERNEL_FINAL_ADDR 0x100000
void halt() {
  debugf("\nSystem halted.\n");
  while (1) {
    __asm__ volatile("cli; hlt");
  }
}

#define CMDLINE_SIZE 2048
BootParams g_boot_params = {0};

typedef void (*KernelStart)(BootParams boot_params);

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

  MemoryMap memory_map;
  memory_map_detect(&memory_map);

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
  bcd_cmdline_construct(bcd.cmdline, strlen(bcd.cmdline), cmdline);

  // Fill BootInfo
  g_boot_params.cpu_info = &cpu_info;
  g_boot_params.disk_params = disk_params;
  g_boot_params.cmdline_buffer = cmdline;
  g_boot_params.cmdline_size = CMDLINE_SIZE;
  g_boot_params.memory_map = memory_map;
  g_boot_params.partition_table = partition_table;
  g_boot_params.partitions_count = partitions_count;

  int kernel_size = fat_read_file(bcd.kernel, (void *)KERNEL_FINAL_ADDR);
  if (kernel_size < 0) {
    debugf("ERROR: Failed to load kernel (error code: %d)!\n", kernel_size);
    halt();
  }

  debugf("Kernel loaded successfully, jumping...\n");
  KernelStart kernelStart = (KernelStart)KERNEL_FINAL_ADDR;
  kernelStart(g_boot_params);

  debugf("ERROR: Kernel returned!\n");
}
