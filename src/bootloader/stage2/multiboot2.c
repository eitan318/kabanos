#include "multiboot2.h"
#include "boot/bootparams.h"
#include <string.h>

#define MB2_ALIGN(x) (((x) + 7) & ~7)

// returns total size
void multiboot2_build(uint8_t *buffer, char *cmdline, int module_count,
                      void **modules_start, int *modules_size,
                      char **modules_paths, MemoryMap *memmap) {
  mb2_info_t *mt2_info = (mb2_info_t *)buffer;
  uint8_t *curr_tag = mt2_info->tags;

  //
  // Command line tag
  //
  mb2_tag_string_t *cmd_tag = (mb2_tag_string_t *)curr_tag;
  cmd_tag->tag.type = MB2_TAG_CMDLINE;
  cmd_tag->tag.size = sizeof(*cmd_tag) + strlen(cmdline) + 1;
  strcpy(cmd_tag->string, cmdline);
  curr_tag += MB2_ALIGN(cmd_tag->tag.size);

  //
  // Modules tags
  //
  for (int i = 0; i < module_count; i++) {
    mb2_tag_module_t *mod_tag = (mb2_tag_module_t *)curr_tag;
    mod_tag->tag.type = MB2_TAG_MODULE;
    mod_tag->mod_start = (uint32_t)modules_start[i];
    mod_tag->mod_end =
        (uint32_t)((uint8_t *)modules_start[i] + modules_size[i]);
    strcpy(mod_tag->cmdline, modules_paths[i]); // copy string into inline array
    mod_tag->tag.size =
        sizeof(*mod_tag) + strlen(modules_paths[i]) + 1; // +1 for '\0'
    curr_tag += MB2_ALIGN(mod_tag->tag.size);
  }

  //
  // Memory map tag
  //
  mb2_tag_mmap_t *mmap_tag = (mb2_tag_mmap_t *)curr_tag;
  mmap_tag->tag.type = MB2_TAG_MMAP;
  mmap_tag->entry_size = sizeof(mb2_mmap_entry_t);
  mmap_tag->entry_version = 0;

  // Copy memory regions
  mb2_mmap_entry_t *entry_ptr = mmap_tag->entries;
  for (int i = 0; i < memmap->region_count; i++) {
    MemoryRegion *r = &memmap->regions[i];
    entry_ptr->addr = r->base;
    entry_ptr->len = r->length;
    entry_ptr->type = r->type;
    entry_ptr->reserved = 0;
    entry_ptr++;
  }

  // Set mmap tag total size
  mmap_tag->tag.size =
      sizeof(*mmap_tag) + sizeof(mb2_mmap_entry_t) * memmap->region_count;
  curr_tag += MB2_ALIGN(mmap_tag->tag.size);

  //
  // End tag
  //
  mb2_tag_t *end_tag = (mb2_tag_t *)curr_tag;
  end_tag->type = MB2_TAG_END;
  end_tag->size = sizeof(*end_tag);
  curr_tag += MB2_ALIGN(end_tag->size);

  mt2_info->total_size = curr_tag - buffer;
}

void multiboot2_jump_to_kernel(void *kernel_entry, uint8_t *multiboot2_info) {
  __asm__ volatile("movl %0, %%ebx\n"          // set ebx first
                   "movl $0x36d76289, %%eax\n" // then eax
                   "jmp *%1\n"                 // jump to kernel entry
                   :
                   : "r"(multiboot2_info), "r"(kernel_entry)
                   : "eax", "ebx", "edx");
}
