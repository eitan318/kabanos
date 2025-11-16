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

#define KERNEL_LOAD_ADDR 0x100000
#define INITRD_LOAD_ADDR 0x200000
#define MODULE_LOAD_ADDR 0x300000
#define CMDLINE_SIZE 2048

void halt() {
  debugf("\nSystem halted.\n");
  while (1) {
    __asm__ volatile("cli; hlt");
  }
}

BootParams g_boot_params = {0};

typedef void (*KernelStart)(BootParams boot_params);

void __attribute__((cdecl)) start(uint32_t boot_drive) {
  i686_gdt_init();

  // Initialize disk
  DiskParams disk_params = {0};
  if (!disk_init(boot_drive, &disk_params)) {
    debugf("ERROR: Failed to initialize disk!\n");
    halt();
  }

  // Read MBR
  PartitionTable partition_table;
  bool res = mbr_partition_table_get(&disk_params, &partition_table);

  MBRPartitionEntry *boot_partition_entry;

  int active_count = 0;
  for (int i = 0; i < 4; i++) {
    if (partition_table.partition_entries[i].boot_flag == BOOTABLE) {
      active_count++;
      boot_partition_entry = &partition_table.partition_entries[i];
    }
  }

  if (active_count > 1) {
    debugf("WARNING: multiple active partitions!\n");
  }

  Partition boot_partition;
  boot_partition.partitionOffset = boot_partition_entry->lba_start;
  boot_partition.partitionSize = boot_partition_entry->total_sectors;
  boot_partition.disk = &disk_params;

  if (!fat_initialize(&boot_partition)) {
    debugf("ERROR: Failed to initialize FAT filesystem!\n");
    halt();
  }

  MemoryMap memory_map;
  memory_map_detect(&memory_map);

  CPUInfo cpu_info;
  collect_cpu_info(&cpu_info);

  char config_buffer[2048];
  int config_size = fat_read_file("/boot.cfg", config_buffer);
  if (config_size < 0) {
    debugf("ERROR: Failed to read config (error code: %d)!\n", config_size);
    halt();
  }

  BCD bcd;
  bcd_parse_into(config_buffer, &bcd);
  char cmdline[CMDLINE_SIZE];
  bcd_cmdline_construct(bcd.cmdline, strlen(bcd.cmdline), cmdline);

  int initrd_size = fat_read_file(bcd.initrd, (void *)INITRD_LOAD_ADDR);
  if (initrd_size < 0) {
    debugf("ERROR: Failed to load initrd (error code: %d)!\n", initrd_size);
    halt();
  }
  //
  // void *module_load_addr = (void *)MODULE_LOAD_ADDR;
  //
  // for (int i = 0; i < bcd.module_count; i++) {
  //   uint32_t size;
  //   int res = fat_read_file(bcd.modules[i].path, (void *)module_load_addr);
  //   g_boot_params.modules[i].start = module_load_addr;
  //   g_boot_params.modules[i].end = (void *)((uint8_t *)module_load_addr +
  //   size); g_boot_params.modules[i].path = bcd.modules[i].path;
  //
  //   module_load_addr += size; // move next module in memory
  // }
  // g_boot_params.module_count = bcd.module_count;

  // Fill BootInfo
  g_boot_params.cpu_info = &cpu_info;
  g_boot_params.disk_params = disk_params;
  g_boot_params.cmdline_buffer = cmdline;
  g_boot_params.cmdline_size = CMDLINE_SIZE;
  g_boot_params.memory_map = memory_map;
  g_boot_params.partition_table = partition_table;

  int kernel_size = fat_read_file(bcd.kernel, (void *)KERNEL_LOAD_ADDR);
  if (kernel_size < 0) {
    debugf("ERROR: Failed to load kernel (error code: %d)!\n", kernel_size);
    halt();
  }

  debugf("Kernel loaded successfully, jumping...\n");
  KernelStart kernelStart = (KernelStart)KERNEL_LOAD_ADDR;
  kernelStart(g_boot_params);

  debugf("ERROR: Kernel returned!\n");
}
