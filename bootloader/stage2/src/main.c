#include "bcd.h"
#include "boot/bootparams.h"
#include "elf.h"
#include "fat.h"
#include "gdt.h"
#include "mbr.h"
#include "memdefs.h"
#include "memory_map.h"
#include "multiboot2.h"
#include "s2lib/stdio.h"
#include "s2lib/string.h"
#include "stdint.h"

#define CMDLINE_SIZE 2048

void halt() {
  debugf("\nSystem halted.\n");
  while (1) {
    __asm__ volatile("cli; hlt");
  }
}

void __attribute__((cdecl)) start(uint32_t boot_drive) {
  i686_gdt_init();

  // Initialize disk
  DiskParams disk_params = {0};
  if (!disk_init(boot_drive, &disk_params)) {
    debugf("ERROR: Failed to initialize disk!\n");
    halt();
  }

  // Read MBR
  partition_table_t partition_table;
  bool res = mbr_partition_table_get(&disk_params, &partition_table);

  mbr_partition_entry_t *boot_partition_entry;

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

  MemoryMapInternal memory_map;
  memory_map_detect(&memory_map);

  FAT_File *boot_cfg_file = fat_open(&boot_partition, "/boot.cfg");

  char config_buffer[boot_cfg_file->size];
  int config_size = fat_read(&boot_partition, boot_cfg_file,
                             boot_cfg_file->size, config_buffer);
  if (config_size < 0) {
    fat_close(boot_cfg_file);
    debugf("ERROR: Failed to read config (error code: %d)!\n", config_size);
    halt();
  }
  fat_close(boot_cfg_file);

  BCD bcd;
  bcd_parse_into(config_buffer, &bcd);

  FAT_File *initrd_file = fat_open(&boot_partition, bcd.initrd);
  // Read initrd, not passed to kernel for now
  int initrd_size = fat_read(&boot_partition, initrd_file, initrd_file->size,
                             (void *)INITRD_LOAD_ADDR);

  if (initrd_size < 0) {
    fat_close(initrd_file);
    debugf("ERROR: Failed to load initrd (error code: %d)!\n", initrd_size);
    halt();
  }
  fat_close(initrd_file);

  void *module_load_addr = (void *)MODULE_LOAD_ADDR;
  uint32_t modules_count = 0;
  void *modules_starts[MAX_MODULES];
  int modules_sizes[MAX_MODULES];

  for (int i = 0; i < bcd.module_count; i++) {
    debugf("Loading module %d: %s\n", i, bcd.modules_paths[i]);

    FAT_File *module_file = fat_open(&boot_partition, bcd.modules_paths[i]);

    int module_size = fat_read(&boot_partition, module_file, module_file->size,
                               module_load_addr);
    if (module_size < 0) {
      debugf("WARNING: Failed to load module %s (error: %d)\n",
             bcd.modules_paths[i], module_size);
      fat_close(module_file);
      continue;
    }
    fat_close(module_file);

    debugf("Module %d loaded at 0x%p, size: %d bytes\n", i, module_load_addr,
           module_size);

    modules_sizes[i] = module_size;
    modules_starts[i] = (void *)module_load_addr;

    // Align next module to 4KB boundary
    uint32_t aligned_size = (module_size + 0xFFF) & ~0xFFF;
    module_load_addr = (void *)((uint8_t *)module_load_addr + aligned_size);

    modules_count++;
  }

  // load kernel
  void *kernel_entry;
  if (!elf_read(&boot_partition, bcd.kernel, &kernel_entry)) {
    printf("ELF read failed, booting halted!");
    halt();
  }

  debugf("Kernel loaded successfully, jumping...\n");

  // Write to the agreed-upon physical address
  uint8_t *multiboot2_info_buffer = (uint8_t *)BOOT_PARAMS_PHYSICAL_ADDR;
  multiboot2_build(multiboot2_info_buffer, bcd.cmdline, modules_count,
                   modules_starts, modules_sizes, bcd.modules_paths,
                   &memory_map);

  multiboot2_jump_to_kernel(kernel_entry, multiboot2_info_buffer);

  // Should never reach here
  debugf("ERROR: Kernel returned!\n");
  halt();
}
