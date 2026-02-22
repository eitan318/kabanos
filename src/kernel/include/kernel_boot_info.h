#pragma once
#include "boot/bootparams.h"
#include "mm/memory_map.h"
#include "modules.h"
#include <stddef.h>

typedef struct {
  char *cmdline;
  module_t *modules;
  int module_count;
  MemoryMap memory_map;
  uint32_t initrd_start;
  uint32_t initrd_size;
} KernelBootInfo;

KernelBootInfo *parse_multiboot2_early(mb2_info_t *mbi);
Range *get_unusable_memory_ranges(KernelBootInfo *kbi, Range memory_range,
                                  size_t *out_count);
