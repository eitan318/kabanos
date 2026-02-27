#include "adt/range.h"
#include "boot/bootparams.h"
#include "fat.h"
#include "kernel_boot_info.h"
#include "klib/stdio.h"
#include "mm/kmalloc.h"
#include "mm/memdefs.h"
#include "mm/memory_map.h"
#include "mm/pmm.h"
#include "mm/vmspace.h"
#include "modules.h"
#include "partition.h"
#include "proc/exec.h"
#include "sched/sched.h"
#include "sched/thread.h"
#include "string.h"
#include "ut/ata_ut_main.h"
#include "ut/frame_allocator_ut_main.h"
#include "ut/print_boot_info.h"
#include "vfs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

vmspace_t g_kernel_vmspace_obj;
vmspace_t *g_kernel_vmspace = &g_kernel_vmspace_obj;
Range g_kernel_virt_range;
Range g_kernel_phys_range;

void kmain(uint32_t mb2_ptr) {
  kdebugf("[Kernel starting...]\n");

  extern uint8_t _kernel_start[], _kernel_end[];

  g_kernel_virt_range.start = (uintptr_t)&_kernel_start;
  g_kernel_virt_range.end = (uintptr_t)&_kernel_end;
  g_kernel_phys_range.start = g_kernel_virt_range.start - KERNEL_BASE;
  g_kernel_phys_range.end = g_kernel_virt_range.end - KERNEL_BASE;

  KernelBootInfo *kernel_boot_info =
      parse_multiboot2_early((mb2_info_t *)mb2_ptr);
  mb2_ptr = 0; // disabling use of unparsed, low half params

  Range total_memory_range = get_memory_range(&kernel_boot_info->memory_map);
  size_t used_memory_ranges_count;
  Range *used_memory_ranges = get_used_memory_ranges(
      kernel_boot_info, total_memory_range, &used_memory_ranges_count);

  size_t useable_memory_ranges_count;
  Range *useable_memory_ranges =
      get_useable_memory_ranges(kernel_boot_info, &useable_memory_ranges_count);

  print_memory_map(kernel_boot_info->memory_map);

  // From now on, no early pmm
  pmm_init(total_memory_range, useable_memory_ranges,
           useable_memory_ranges_count, used_memory_ranges,
           used_memory_ranges_count);

  kernel_vmspace_create(g_kernel_vmspace, total_memory_range);
  if (g_kernel_vmspace == NULL) {
    kdebugf("FAIL: Could not create page directory\n");
    return;
  }
  // From now on no lower half mapping
  vmspace_switch(g_kernel_vmspace);

  kmalloc_init();

  // Load static and dynamic modules
  modules_init_registry(kernel_boot_info->modules);
  modules_load();

  partition_table_t partition_table;
  bool res = kmbr_partition_table_get(&partition_table);

  MBRPartitionEntry *boot_partition_entry;

  int active_count = 0;
  for (int i = 0; i < 4; i++) {
    if (partition_table.partition_entries[i].boot_flag == BOOTABLE) {
      active_count++;
      boot_partition_entry = &partition_table.partition_entries[i];
    }
  }

  // my invention
  vfs_mount("dev", "/", "FAT12", 0, NULL);

  if (!fat_initialize(boot_partition_entry->lba_start)) {
    kdebugf("Failed to initialize FAT\n");
    for (;;) {
    }
  }

  process_spawn("init.elf", PRIORITY_VERY_HIGH);

  hal_timer_enable();

  sched_yield();

  kprintf("problematic return");

  for (;;) {
  }
}
