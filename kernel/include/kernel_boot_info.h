#pragma once
#include "boot/bootparams.h"
#include "klib/stddef.h"
#include "mm/memory_map.h"
#include "modules.h"

typedef struct {
  char *cmdline;
  module_t *modules;
  int module_count;
  memory_map_t memory_map;
  uint32_t initrd_start;
  uint32_t initrd_size;
} KernelBootInfo;

KernelBootInfo *parse_multiboot2_early(mb2_info_t *mbi);
range_t *get_used_memory_ranges(KernelBootInfo *kbi, range_t memory_range,
                                size_t *out_count);

range_t *get_useable_memory_ranges(KernelBootInfo *kbi, size_t *out_count);
