#include "kernel_boot_info.h"
#include "adt/range.h"
#include "boot/bootparams.h"
#include "mm/memory_map.h"
#include "string.h"
#include "utils/math.h"

#define EARLYALLOC_SIZE (5 * 1024)

static void *early_alloc(unsigned len) {
  static uint8_t buf[EARLYALLOC_SIZE];
  static unsigned idx = 0;

  if (idx + len >= EARLYALLOC_SIZE) {
    /* Return NULL on failure. It's too early in the boot process to give out a
       diagnostic.*/
    return NULL;
  }
  uint8_t *ptr = &buf[idx];
  idx += len;

  return ptr;
}

KernelBootInfo *parse_multiboot2_early(mb2_info_t *mbi) {

  KernelBootInfo *kernel_boot_info =
      (KernelBootInfo *)early_alloc(sizeof(*kernel_boot_info));
  uint8_t *tag_ptr = mbi->tags;

  kernel_boot_info->module_count = 0;

  while (tag_ptr < ((uint8_t *)mbi + mbi->total_size)) {
    mb2_tag_t *tag = (mb2_tag_t *)tag_ptr;

    switch (tag->type) {
    case MB2_TAG_CMDLINE: {
      mb2_tag_string_t *cmd_tag = (void *)tag;
      const char *cmdline_src = cmd_tag->string;

      // Allocate/copy into kernel memory
      size_t len = strlen(cmdline_src) + 1;
      kernel_boot_info->cmdline = (char *)early_alloc(len);
      memcpy(kernel_boot_info->cmdline, cmdline_src, len);
      break;
    }

    case MB2_TAG_MODULE: {
      mb2_tag_module_t *mod_tag = (void *)tag;

      module_t *mod = early_alloc(sizeof(module_t));
      memset(mod, 0, sizeof(module_t));

      mod->data_start = (void *)mod_tag->mod_start;
      mod->data_size = mod_tag->mod_end - mod_tag->mod_start;

      size_t len = strlen(mod_tag->cmdline) + 1;
      char *cmdline = early_alloc(len);
      memcpy(cmdline, mod_tag->cmdline, len);

      mod->name = cmdline;

      mod->next = kernel_boot_info->modules;
      kernel_boot_info->modules = mod;

      kernel_boot_info->module_count++;
      break;
    }
    case MB2_TAG_MMAP: {
      mb2_tag_mmap_t *mmap_tag = (void *)tag;
      mb2_mmap_entry_t *entry;

      for (entry = mmap_tag->entries;
           (uint8_t *)entry < ((uint8_t *)mmap_tag + mmap_tag->tag.size);
           entry =
               (mb2_mmap_entry_t *)((uint8_t *)entry + mmap_tag->entry_size)) {
        if (kernel_boot_info->memory_map.region_count < MAX_MEMORY_REGIONS) {
          memory_region_t *r =
              &kernel_boot_info->memory_map
                   .regions[kernel_boot_info->memory_map.region_count++];
          r->start = entry->addr;
          r->size = entry->len;
          r->type = entry->type;
        }
      }
      break;
    }

    case MB2_TAG_END:
      return kernel_boot_info;
    }

    tag_ptr += align_up(tag->size, 8);
  }
  return kernel_boot_info;
}

Range *get_useable_memory_ranges(KernelBootInfo *kbi, size_t *out_count) {
  static range_list_t usable_list;
  usable_list.count = 0;

  for (size_t i = 0; i < kbi->memory_map.region_count; i++) {
    memory_region_t *region = &kbi->memory_map.regions[i];
    if (region->type == 1) { // 1 = USABLE_RAM
      range_list_push(
          &usable_list,
          (Range){.start = region->start, .end = region->start + region->size});
    }
  }
  *out_count = usable_list.count;
  return usable_list.ranges;
}

Range *get_used_memory_ranges(KernelBootInfo *kbi, Range memory_range,
                              size_t *out_count) {
  static range_list_t list;
  list.count = 0;

  if (kbi->initrd_start && kbi->initrd_size) {
    range_list_push(&list, (Range){
                               .start = kbi->initrd_start,
                               .end = kbi->initrd_start + kbi->initrd_size,
                           });
  }

  for (int i = 0; i < kbi->module_count; i++) {
    module_t *m = &kbi->modules[i];
    range_list_push(&list, (Range){
                               .start = (uintptr_t)m->data_start,
                               .end = (uintptr_t)m->data_start + m->data_size,
                           });
  }

  extern Range g_kernel_phys_range;
  range_list_push(&list, g_kernel_phys_range);

  *out_count = list.count;
  return list.ranges;
}
