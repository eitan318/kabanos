#include "arch/i686/vga_text.h"
#include "boot/bootparams.h"
#include "fat/fat.h"
#include "hal/hal.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "initrd/initrd.h"
#include "memory_management/boot_allocator.h"
#include "memory_management/kmalloc.h"
#include "memory_management/pmm.h"
#include "memory_management/vmm.h"
#include "modules/modules.h"
#include "process/pcb.h"
#include "process/schedualer.h"
#include "process/task.h"
#include "ut/ata/ata_ut_main.h"
#include "ut/keyboard_driver.h"
#include "ut/paging/paging_ut_main.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

vmspace_t *g_kernel_vmspace;
extern uint8_t _kernel_start[], _kernel_end[], _bss_start[];
#define MAX_USED_RANGES 64

typedef struct {
  Range ranges[MAX_USED_RANGES];
  size_t count;
} RangeList;

static inline void range_list_push(RangeList *list, Range r) {
  if (list->count < MAX_USED_RANGES) {
    list->ranges[list->count++] = r;
  }
}

static void collect_non_usable_ranges(RangeList *list, MemoryMap *memory_map,
                                      Range memory_range) {
  for (int i = 0; i < memory_map->region_count; i++) {
    MemoryRegion *region = &memory_map->regions[i];

    if (region->type == E820_USABLE)
      continue;

    Range r = {
        .start = region->base,
        .end = region->base + region->length,
    };

    r = range_align_outward(r, FRAME_SIZE);
    r = range_clamp(r, memory_range);

    if (r.start < r.end)
      range_list_push(list, r);
  }
}
static void collect_kernel_range(RangeList *list) {
  Range r = {
      .start = (uint64_t)&_kernel_start,
      .end = (uint64_t)&_kernel_end,
  };

  range_list_push(list, r);
}
static void collect_initrd_range(RangeList *list, BootParams *bp) {
  if (bp->initrd_start && bp->initrd_size > 0) {
    Range r = {
        .start = (uint64_t)bp->initrd_start,
        .end = (uint64_t)bp->initrd_start + bp->initrd_size,
    };
    range_list_push(list, r);
  }
}

static void collect_module_ranges(RangeList *list, BootParams *bp) {
  for (uint32_t i = 0; i < bp->module_count; i++) {
    Module *m = &bp->modules[i];

    if (m->start && m->size > 0) {
      Range r = {
          .start = (uint64_t)m->start,
          .end = (uint64_t)m->start + m->size,
      };
      range_list_push(list, r);
    }
  }
}
static void collect_cmdline_range(RangeList *list, BootParams *bp) {
  if (bp->cmdline_buffer && bp->cmdline_size > 0) {
    Range r = {
        .start = (uint64_t)bp->cmdline_buffer,
        .end = (uint64_t)bp->cmdline_buffer + bp->cmdline_size,
    };
    range_list_push(list, r);
  }
}

static Range *get_unusable_memory_ranges(BootParams *boot_params,
                                         MemoryMap *memory_map,
                                         Range memory_range,
                                         size_t *out_count) {
  static RangeList list;
  list.count = 0;

  collect_non_usable_ranges(&list, memory_map, memory_range);
  collect_kernel_range(&list);
  collect_initrd_range(&list, boot_params);
  collect_module_ranges(&list, boot_params);
  collect_cmdline_range(&list, boot_params);
  range_list_push(&list, boot_alloc_get_used_range());

  *out_count = list.count;
  return list.ranges;
}

static Range get_memory_range(MemoryMap *memory_map) {
  uint64_t max_addr = 0;          // Initialize to 0
  uint64_t min_addr = UINT64_MAX; // Initialize to max value

  for (int i = 0; i < memory_map->region_count; i++) {
    MemoryRegion *region = &memory_map->regions[i];

    if (region->type != E820_USABLE)
      continue;

    uint64_t region_start = region->base;
    uint64_t region_end = region->base + region->length;

    if (region_start < min_addr)
      min_addr = region_start;
    if (region_end > max_addr)
      max_addr = region_end;
  }

  Range range = {
      .start = min_addr,
      .end = max_addr,
  };

  return range;
}

void __attribute__((section(".entry"))) start(BootParams boot_params) {
  memset(_bss_start, 0, _kernel_end - _bss_start);
  vga_clrscr();
  vga_setcursor(0, 0);

  debugf("[Kernel starting...]\n");

  // Initialize initrd
  if (boot_params.initrd_start && boot_params.initrd_size > 0) {
    initrd_init(boot_params.initrd_start, boot_params.initrd_size);
    initrd_list_files();
  } else {
    debugf("No initrd provided\n");
  }

  // Initialize modules
  if (boot_params.module_count > 0) {
    modules_init(&boot_params);
  } else {
    debugf("No modules provided\n");
  }

  hal_init();

  __asm__ volatile("sti");

  g_kernel_vmspace = kernel_vmspace_creat();
  if (g_kernel_vmspace == NULL) {
    debugf("FAIL: Could not create page directory\n");
    return;
  }

  vmspace_switch(g_kernel_vmspace);

  Range total_memory_range = get_memory_range(&boot_params.memory_map);
  size_t count;
  Range *unusable_memory_ranges = get_unusable_memory_ranges(
      &boot_params, &boot_params.memory_map, total_memory_range, &count);
  pmm_init(total_memory_range, unusable_memory_ranges, count);

  ut_paging_main(&boot_params);

  kmalloc_init();

  if (!fat_initialize(34, 0)) {
    debugf("Failed to initialize FAT\n");
    for (;;) {
    }
  }

  debugf("Testing tasks\n");
  test_tasks();
  for (;;) {
  }
}
