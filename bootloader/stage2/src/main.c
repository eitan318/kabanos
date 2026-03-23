#include "bcd.h"
#include "boot/bootparams.h"
#include "elf.h"
#include "fat.h"
#include "gdt.h"
#include "malloc.h"
#include "mbr.h"
#include "memdefs.h"
#include "memory_map.h"
#include "multiboot2.h"
#include "s2lib/stdio.h"
#include "stdint.h"
#include "vbe.h"

#define CMDLINE_SIZE 2048

/** Disables interrupts and halts the CPU indefinitely. */
void halt() {
  debugf("\nSystem halted.\n");
  while (1) {
    __asm__ volatile("cli; hlt");
  }
}

/**
 * Loads the initrd image specified in @p bcd into INITRD_LOAD_ADDR.
 * Halts on read failure.
 *
 * @param boot_partition Mounted FAT partition to read from.
 * @param bcd            Boot configuration descriptor with the initrd path.
 */
static void load_initrd(partition_t *boot_partition, BCD *bcd) {
  fat_file *initrd_file = fat_open(boot_partition, bcd->initrd);
  int initrd_size = fat_read(boot_partition, initrd_file, initrd_file->size,
                             (void *)INITRD_LOAD_ADDR);
  if (initrd_size < 0) {
    fat_close(initrd_file);
    debugf("ERROR: Failed to load initrd (error code: %d)!\n", initrd_size);
    halt();
  }
  fat_close(initrd_file);
}

/**
 * Initializes the boot disk, selects the active partition, and mounts the FAT
 * filesystem. Halts on any failure.
 *
 * @param boot_drive     BIOS drive number passed from the bootloader.
 * @param disk_params    Output: populated disk parameter structure.
 * @param boot_partition Output: populated partition structure ready for FAT
 * I/O.
 */
static void init_disk_and_partition(uint32_t boot_drive,
                                    disk_params_t *disk_params,
                                    partition_t *boot_partition) {
  if (!disk_init(boot_drive, disk_params)) {
    debugf("ERROR: Failed to initialize disk!\n");
    halt();
  }

  partition_table_t partition_table;
  mbr_partition_table_get(disk_params, &partition_table);

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

  // Copy values out before partition_table leaves scope
  boot_partition->partitionOffset = boot_partition_entry->lba_start;
  boot_partition->partitionSize = boot_partition_entry->total_sectors;
  boot_partition->disk = disk_params;

  if (!fat_initialize(boot_partition)) {
    debugf("ERROR: Failed to initialize FAT filesystem!\n");
    halt();
  }
}

/**
 * Reads and parses /boot.cfg from the boot partition into @p bcd.
 * Halts if the file cannot be read.
 *
 * @param boot_partition Mounted FAT partition to read from.
 * @param bcd            Output: populated boot configuration descriptor.
 */
static void load_config(partition_t *boot_partition, BCD *bcd) {
  fat_file *boot_cfg_file = fat_open(boot_partition, "/boot.cfg");

  char *config_buffer = malloc(boot_cfg_file->size);

  // char config_buffer[boot_cfg_file->size];
  int config_size = fat_read(boot_partition, boot_cfg_file, boot_cfg_file->size,
                             config_buffer);
  if (config_size < 0) {
    fat_close(boot_cfg_file);
    debugf("ERROR: Failed to read config (error code: %d)!\n", config_size);
    halt();
  }
  fat_close(boot_cfg_file);

  bcd_parse_into(config_buffer, bcd);
}

/**
 * Loads all modules listed in @p bcd into consecutive 4KB-aligned regions
 * starting at MODULE_LOAD_ADDR. Modules that fail to load are skipped with a
 * warning.
 *
 * @param boot_partition  Mounted FAT partition to read from.
 * @param bcd             Boot configuration descriptor with module paths.
 * @param modules_starts  Output: base address of each successfully loaded
 * module.
 * @param modules_sizes   Output: byte size of each successfully loaded module.
 * @return                Number of modules successfully loaded.
 */
static uint32_t load_modules(partition_t *boot_partition, BCD *bcd,
                             void **modules_starts, int *modules_sizes) {
  void *module_load_addr = (void *)MODULE_LOAD_ADDR;
  uint32_t modules_count = 0;

  for (int i = 0; i < bcd->module_count; i++) {
    debugf("Loading module %d: %s\n", i, bcd->modules_paths[i]);

    fat_file *module_file = fat_open(boot_partition, bcd->modules_paths[i]);
    int module_size = fat_read(boot_partition, module_file, module_file->size,
                               module_load_addr);
    if (module_size < 0) {
      debugf("WARNING: Failed to load module %s (error: %d)\n",
             bcd->modules_paths[i], module_size);
      fat_close(module_file);
      continue;
    }
    fat_close(module_file);

    debugf("Module %d loaded at 0x%p, size: %d bytes\n", i, module_load_addr,
           module_size);

    modules_sizes[i] = module_size;
    modules_starts[i] = module_load_addr;

    uint32_t aligned_size = (module_size + 0xFFF) & ~0xFFF;
    module_load_addr = (void *)((uint8_t *)module_load_addr + aligned_size);

    modules_count++;
  }

  return modules_count;
}

/**
 * Stage 2 bootloader entry point. Initializes hardware, loads the kernel and
 * its supporting files, builds the Multiboot2 info structure, and transfers
 * control to the kernel. Never returns.
 *
 * @param boot_drive BIOS drive number provided by stage 1.
 */
void __attribute__((cdecl)) start(uint32_t boot_drive) {
  i686_gdt_init();

  disk_params_t disk_params = {0};
  partition_t boot_partition;
  init_disk_and_partition(boot_drive, &disk_params, &boot_partition);

  MemoryMapInternal memory_map;
  memory_map_detect(&memory_map);

  BCD bcd;
  load_config(&boot_partition, &bcd);
  load_initrd(&boot_partition, &bcd);

  void *modules_starts[MAX_MODULES];
  int modules_sizes[MAX_MODULES];
  uint32_t modules_count =
      load_modules(&boot_partition, &bcd, modules_starts, modules_sizes);

  void *kernel_entry;
  if (!elf_read(&boot_partition, bcd.kernel, &kernel_entry)) {
    printf("kernel ELF read failed, booting halted!");
    halt();
  }

  vbe_init();

  debugf("Kernel loaded successfully, jumping...\n");

  uint8_t *multiboot2_info_buffer = (uint8_t *)BOOT_PARAMS_PHYSICAL_ADDR;
  multiboot2_build(multiboot2_info_buffer, bcd.cmdline, modules_count,
                   modules_starts, modules_sizes, bcd.modules_paths,
                   &memory_map);

  multiboot2_jump_to_kernel(kernel_entry, multiboot2_info_buffer);

  debugf("ERROR: Kernel returned!\n");
  halt();
}
