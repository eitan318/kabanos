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
#define MODULE_PATH_SIZE 256
#define MODULES_MAX 16

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

  g_boot_params.initrd_start = (void *)INITRD_LOAD_ADDR;
  g_boot_params.initrd_size = (uint32_t)initrd_size;
  debugf("Initrd loaded at 0x%x, size: %d bytes\n", INITRD_LOAD_ADDR,
         initrd_size);

  // Load modules
  void *module_load_addr = (void *)MODULE_LOAD_ADDR;
  g_boot_params.module_count = 0;

  for (int i = 0; i < bcd.module_count; i++) {
    debugf("Loading module %d: %s\n", i, bcd.modules[i].path);

    int module_size = fat_read_file(bcd.modules[i].path, module_load_addr);
    if (module_size < 0) {
      debugf("WARNING: Failed to load module %s (error: %d)\n",
             bcd.modules[i].path, module_size);
      continue;
    }

    // Store module information
    g_boot_params.modules[i].start = module_load_addr;
    g_boot_params.modules[i].size = (uint32_t)module_size;

    // Copy path string to a safe location (heap or static buffer)
    // Instead of storing pointer to BCD data which may be overwritten
    static char module_paths[MAX_MODULES][MODULE_PATH_SIZE];
    strncpy(module_paths[i], bcd.modules[i].path, MODULE_PATH_SIZE - 1);
    module_paths[i][MODULE_PATH_SIZE - 1] = '\0';
    g_boot_params.modules[i].path = module_paths[i];

    debugf("Module %d loaded at 0x%p, size: %d bytes\n", i, module_load_addr,
           module_size);

    // Align next module to 4KB boundary
    uint32_t aligned_size = (module_size + 0xFFF) & ~0xFFF;
    module_load_addr = (void *)((uint8_t *)module_load_addr + aligned_size);

    g_boot_params.module_count++;
  }

  // Fill remaining BootParams
  g_boot_params.cpu_info = cpu_info;
  g_boot_params.disk_params = disk_params;
  g_boot_params.cmdline_buffer = cmdline;
  g_boot_params.cmdline_size = CMDLINE_SIZE;
  g_boot_params.memory_map = memory_map;
  g_boot_params.partition_table = partition_table;

  // load kernel
  KernelStart kernelEntry;
  if (!elf_read(&boot_partition, bcd.kernel, (void **)&kernelEntry)) {
    printf("ELF read failed, booting halted!");
    halt();
  }

  debugf("Kernel loaded successfully, jumping...\n");
  debugf("Kernel entry point address: 0x%x\n", (uint32_t)kernelEntry);
  debugf("Expected address should be: 0x%x\n", KERNEL_LOAD_ADDR);
  
  kernelEntry(g_boot_params);

  // Should never reach here
  debugf("ERROR: Kernel returned!\n");
  halt();
}
