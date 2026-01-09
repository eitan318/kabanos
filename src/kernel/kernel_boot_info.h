#pragma once
#include "boot/bootparams.h"
#include "memory_map.h"
#include <stddef.h>

typedef struct {
  void *start;
  size_t size;
  char *cmdline; // copy of module path or parameters
  bool loaded;
} KernelModule;

typedef struct {
  char *cmdline;
  KernelModule modules[MAX_MODULES];
  int module_count;
  MemoryMap memory_map;
  uint32_t initrd_start;
  uint32_t initrd_size;
} KernelBootInfo;

KernelBootInfo parse_multiboot2_early(mb2_info_t *mbi);
Range *get_unusable_memory_ranges(KernelBootInfo *kbi, Range memory_range,
                                  size_t *out_count);
