#include "loader/elf.h"
#include "fat/fat.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "include/string.h"
#include "memory_management/kmalloc.h"
#include "memory_management/kmap.h"
#include "memory_management/pmm.h"
#include "memory_management/va_allocation.h"
#include "memory_management/vmm.h"
#include "memory_management/vmspace.h"
#include "utils/math.h"

// Load segment: allocate, zero, and copy from file data
static int load_segment(page_dir_t *pd, vaddr_t va, size_t mem_size,
                        void *file_data, size_t file_size, uint32_t flags) {
  /* Allocate pages */
  if (!va_alloc_region(pd, va, mem_size, flags)) {
    return -1;
  }

  /* Process page by page */
  vaddr_t va_end = va + mem_size;
  size_t file_offset = 0;

  for (vaddr_t page_va = va & ~(PAGE_SIZE - 1);
       page_va < align_up(va_end, PAGE_SIZE); page_va += PAGE_SIZE) {

    paddr_t phys = virt_to_phys(pd, page_va);
    if (!phys)
      return -1;

    void *kva = kmap(phys);
    if (!kva)
      return -1;

    /* Calculate byte range in this page */
    size_t page_start = (page_va < va) ? (va - page_va) : 0;
    size_t page_end =
        ((page_va + PAGE_SIZE) > va_end) ? (va_end - page_va) : PAGE_SIZE;

    /* Zero entire range first */
    memset((uint8_t *)kva + page_start, 0, page_end - page_start);

    /* Copy file data if available */
    if (file_offset < file_size) {
      size_t bytes_to_copy = file_size - file_offset;
      size_t space_in_page = page_end - page_start;
      if (bytes_to_copy > space_in_page) {
        bytes_to_copy = space_in_page;
      }

      memcpy((uint8_t *)kva + page_start, (uint8_t *)file_data + file_offset,
             bytes_to_copy);
      file_offset += bytes_to_copy;
    }

    kunmap();
  }

  return 0;
}

/* ============================================================================
 * Load ELF file
 * ============================================================================
 */
int elf_load(page_dir_t *pd, void *elf_data, uint32_t elf_size,
             uintptr_t *entry) {
  if (!pd || !elf_data || !entry || elf_size < sizeof(ELFHeader)) {
    return -1;
  }

  ELFHeader *hdr = (ELFHeader *)elf_data;

  /* Validate ELF header */
  if (memcmp(hdr->Magic, ELF_MAGIC, 4) != 0 ||
      hdr->Bitness != ELF_BITNESS_32BIT ||
      hdr->Endianness != ELF_ENDIANNESS_LITTLE ||
      hdr->Type != ELF_TYPE_EXECUTABLE ||
      hdr->InstructionSet != ELF_INSTRUCTION_SET_X86) {
    debugf("ELF: Invalid header\n");
    return -1;
  }

  /* Validate program header table */
  uint32_t phdr_size =
      hdr->ProgramHeaderTableEntryCount * hdr->ProgramHeaderTableEntrySize;
  if (hdr->ProgramHeaderTablePosition > elf_size ||
      hdr->ProgramHeaderTablePosition + phdr_size > elf_size) {
    debugf("ELF: Invalid program headers\n");
    return -1;
  }

  /* Load segments */
  uint8_t *phdrs = (uint8_t *)elf_data + hdr->ProgramHeaderTablePosition;

  for (uint32_t i = 0; i < hdr->ProgramHeaderTableEntryCount; i++) {
    ELFProgramHeader *ph =
        (ELFProgramHeader *)(phdrs + i * hdr->ProgramHeaderTableEntrySize);

    if (ph->Type != ELF_PROGRAM_TYPE_LOAD || ph->MemorySize == 0) {
      continue;
    }

    /* Validate segment bounds */
    if (ph->Offset > elf_size || ph->Offset + ph->FileSize > elf_size) {
      debugf("ELF: Segment %u out of bounds\n", i);
      return -1;
    }

    /* Load segment */
    uint32_t flags =
        PAGE_USER |
        ((ph->Flags & ELF_PROGRAM_FLAG_WRITABLE) ? PAGE_READWRITE : 0);
    void *file_data =
        (ph->FileSize > 0) ? ((uint8_t *)elf_data + ph->Offset) : NULL;

    if (load_segment(pd, ph->VirtualAddress, ph->MemorySize, file_data,
                     ph->FileSize, flags) != 0) {
      debugf("ELF: Failed to load segment %u\n", i);
      return -1;
    }
  }

  *entry = hdr->ProgramEntryPosition;
  return 0;
}
