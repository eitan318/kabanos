#include "kernel_boot_info.h"
#include "boot/bootparams.h"
#include "include/memory.h"
#include "include/string.h"
#include "memory_management/early_pmm.h"
#include "memory_management/memory_map.h"
#include "utils/math.h"
#include "utils/range.h"

KernelBootInfo *parse_multiboot2_early(mb2_info_t *mbi) {

  KernelBootInfo *kernel_boot_info =
      early_pmm_vm_alloc(sizeof(*kernel_boot_info));
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
      kernel_boot_info->cmdline = early_pmm_vm_alloc(len);
      memcpy(kernel_boot_info->cmdline, cmdline_src, len);
      break;
    }

    case MB2_TAG_MODULE: {
      mb2_tag_module_t *mod_tag = (void *)tag;
      if (kernel_boot_info->module_count < MAX_MODULES) {
        KernelModule *mod =
            &kernel_boot_info->modules[kernel_boot_info->module_count++];

        mod->start = (void *)mod_tag->mod_start;
        mod->size = mod_tag->mod_end - mod_tag->mod_start;

        // Copy module cmdline string into kernel memory
        size_t len = strlen(mod_tag->cmdline) + 1;
        mod->cmdline = early_pmm_vm_alloc(len);
        memcpy(mod->cmdline, mod_tag->cmdline, len);
      }
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
          MemoryRegion *r =
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

Range *get_unusable_memory_ranges(KernelBootInfo *kbi, Range memory_range,
                                  size_t *out_count) {
  static RangeList list;
  list.count = 0;

  collect_non_usable_ranges(&list, &kbi->memory_map, memory_range);

  if (kbi->initrd_start && kbi->initrd_size) {
    range_list_push(&list, (Range){
                               .start = kbi->initrd_start,
                               .end = kbi->initrd_start + kbi->initrd_size,
                           });
  }

  for (int i = 0; i < kbi->module_count; i++) {
    KernelModule *m = &kbi->modules[i];
    range_list_push(&list, (Range){
                               .start = (uintptr_t)m->start,
                               .end = (uintptr_t)m->start + m->size,
                           });
  }

  extern Range g_kernel_phys_range;
  range_list_push(&list, g_kernel_phys_range);

  *out_count = list.count;
  return list.ranges;
}
