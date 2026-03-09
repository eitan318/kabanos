#include "elf32.h"
#include "arch/types.h"
#include "assert.h"
#include "hal.h"
#include "klib/stdio.h"
#include "mm/memdefs.h"
#include "mm/pmm.h"
#include "mm/va_allocation.h"
#include "utils/math.h"
#include <klib/stdint.h>

// Load segment: allocate, zero, and copy from file data
// currently menually touch frame alloc and page_map
// In future will be with VMA of process
static int load_segment(arch_vm_t *vm, vaddr_t va_start, size_t mem_size,
                        void *file_data, size_t file_size, uint32_t flags) {
  vaddr_t pages_start = align_down(va_start, PAGE_SIZE);
  vaddr_t pages_end = align_up(va_start + mem_size, PAGE_SIZE);
  size_t alloc_size = pages_end - pages_start;

  if (!va_alloc_region(vm, pages_start, alloc_size, flags)) {
    return -1;
  }

  for (vaddr_t page_va = pages_start; page_va < pages_end;
       page_va += PAGE_SIZE) {
    paddr_t phys = hal_vm_virt_to_phys(vm, page_va);
    void *kva = (void *)(phys + KERNEL_BASE);

    memset(kva, 0, PAGE_SIZE);

    vaddr_t intersect_start = MAX(page_va, va_start);
    vaddr_t intersect_end = MIN(page_va + PAGE_SIZE, va_start + file_size);

    if (intersect_start < intersect_end) {
      size_t dest_offset = intersect_start - page_va;
      size_t src_offset = intersect_start - va_start;
      size_t bytes_to_copy = intersect_end - intersect_start;
      memcpy((uint8_t *)kva + dest_offset, (uint8_t *)file_data + src_offset,
             bytes_to_copy);
    }
  }

  return 0;
}

static const char *get_elf_string(void *elf_data, uint32_t strtab_off,
                                  uint32_t name_idx) {
  return (const char *)((uint8_t *)elf_data + strtab_off + name_idx);
}

// Load ELF file
int elf32_load(arch_vm_t *vm, void *elf_data, uint32_t elf_size,
               uintptr_t *entry, uintptr_t *text_base) {
  ASSERT(vm && elf_data && entry);

  if (elf_size < sizeof(elf32_header_t)) {
    kdebugf("ELF: File too small\n");
    return -1;
  }

  elf32_header_t *hdr = (elf32_header_t *)elf_data;
  // Validate ELF header
  if (memcmp(hdr->magic, ELF_MAGIC, 4) != 0 ||
      hdr->wordsize != ELF_BITNESS_32BIT ||
      hdr->endianness != ELF_ENDIANNESS_LITTLE ||
      hdr->type != ELF_TYPE_EXECUTABLE ||
      hdr->instruction_set != ELF_INSTRUCTION_SET_X86) {
    kdebugf("ELF: Invalid header\n");
    return -1;
  }

  if (text_base) {
    // Find Section Header String Table (.shstrtab)
    elf32_sec_hdr *shdr_table =
        (elf32_sec_hdr *)((uint8_t *)elf_data + hdr->shdr_table_pos);
    elf32_sec_hdr *shstrtab_hdr = &shdr_table[hdr->section_names_index];
    uint32_t shstrtab_off = shstrtab_hdr->offset;

    *text_base = 0; // Default if not found

    for (uint32_t i = 0; i < hdr->shdr_table_entry_count; i++) {
      elf32_sec_hdr *sh = &shdr_table[i];
      const char *name = get_elf_string(elf_data, shstrtab_off, sh->name);

      if (strcmp(name, ".text") == 0) {
        *text_base = sh->addr;
        break;
      }
    }
  }

  // Validate program header table
  uint32_t phdr_size = hdr->phdr_table_entry_count * hdr->phdr_table_entry_size;
  if (hdr->phdr_table_pos > elf_size ||
      hdr->phdr_table_pos + phdr_size > elf_size) {
    kdebugf("ELF: Invalid program headers\n");
    return -1;
  }

  // Load segments
  uint8_t *phdrs = (uint8_t *)elf_data + hdr->phdr_table_pos;

  for (uint32_t i = 0; i < hdr->phdr_table_entry_count; i++) {
    elf32_prog_hdr *ph =
        (elf32_prog_hdr *)(phdrs + i * hdr->phdr_table_entry_size);

    if (ph->type != ELF_PROGRAM_TYPE_LOAD || ph->mem_size == 0) {
      continue;
    }

    // Validate segment bounds
    if (ph->off > elf_size || ph->off + ph->file_size > elf_size) {
      kdebugf("ELF: Segment %u out of bounds\n", i);
      return -1;
    }

    // Load segment
    uint32_t flags =
        PAGE_USER |
        ((ph->flags & ELF_PROGRAM_FLAG_WRITABLE) ? PAGE_READWRITE : 0);
    void *file_data =
        (ph->file_size > 0) ? ((uint8_t *)elf_data + ph->off) : NULL;

    if (load_segment(vm, ph->vaddr, ph->mem_size, file_data, ph->file_size,
                     flags) != 0) {
      kdebugf("ELF: Failed to load segment %u\n", i);
      return -1;
    }
  }

  *entry = hdr->program_entry_pos;
  return 0;
}
