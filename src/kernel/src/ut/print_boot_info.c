#include "kernel_boot_info.h"
#include "klib/stdio.h"

void print_memory_map(MemoryMap memory_map) {
  kdebugf(
      "\nMemory map:\n"
      "Idx | start  |  size  | Type\n"
      "-----------------------------------------------------------------\n");

  for (int i = 0; i < memory_map.region_count; i++) {
    const MemoryRegion *e = &memory_map.regions[i];

    kdebugf("%x | %llx | %llx | %x\n", i, e->start, e->size, e->type);
  }
}
