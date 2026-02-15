#include "loader/elf.h"
#include "arch/types.h"
#include "assert.h"
#include "fat/fat.h"
#include "hal.h"
#include "memory_management/kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/pmm.h"
#include "memory_management/va_allocation.h"
#include "memory_management/vmspace.h"
#include "stdio.h"
#include "string.h"
#include "utils/math.h"
#include "utils/range.h"
#include <stdarg.h>

// Load segment: allocate, zero, and copy from file data
// currently menually touch frame alloc and page_map
// In future will be with VMA of process
static int load_segment(arch_vm_t *vm, vaddr_t va_start, size_t mem_size,
                        void *file_data, size_t file_size, uint32_t flags) {
  if (!va_alloc_region(vm, va_start, mem_size, flags)) {
    return -1;
  }

  vaddr_t pages_start = align_down(va_start, PAGE_SIZE);
  vaddr_t pages_end = align_up(va_start + mem_size, PAGE_SIZE);

  size_t file_copied = 0;
  for (vaddr_t page_va = pages_start; page_va < pages_end;
       page_va += PAGE_SIZE) {
    paddr_t phys = hal_vm_virt_to_phys(vm, page_va);
    ASSERT(phys);

    void *kva = (void *)(phys + KERNEL_BASE);
    ASSERT(kva);

    // Calculate which bytes in this page belong to the segment
    size_t page_offset = (page_va < va_start) ? (va_start - page_va) : 0;
    size_t page_limit = (page_va + PAGE_SIZE > va_start + mem_size)
                            ? (va_start + mem_size - page_va)
                            : PAGE_SIZE;
    size_t seg_bytes_in_page = page_limit - page_offset;

    // Zero the segment's portion of this page
    memset((uint8_t *)kva + page_offset, 0, seg_bytes_in_page);

    // Copy file data if available (overwrites zeros)
    if (file_copied < file_size) {
      size_t bytes_to_copy = file_size - file_copied;
      if (bytes_to_copy > seg_bytes_in_page) {
        bytes_to_copy = seg_bytes_in_page;
      }

      memcpy((uint8_t *)kva + page_offset, (uint8_t *)file_data + file_copied,
             bytes_to_copy);

      file_copied += bytes_to_copy;
    }
  }

  return 0;
}

// Load ELF file
int elf_load(arch_vm_t *vm, void *elf_data, uint32_t elf_size,
             uintptr_t *entry) {
  ASSERT(vm && elf_data && entry);

  if (elf_size < sizeof(ELFHeader)) {
    debugf("ELF: File too small\n");
    return -1;
  }

  ELFHeader *hdr = (ELFHeader *)elf_data;

  // Validate ELF header
  if (memcmp(hdr->Magic, ELF_MAGIC, 4) != 0 ||
      hdr->Bitness != ELF_BITNESS_32BIT ||
      hdr->Endianness != ELF_ENDIANNESS_LITTLE ||
      hdr->Type != ELF_TYPE_EXECUTABLE ||
      hdr->InstructionSet != ELF_INSTRUCTION_SET_X86) {
    debugf("ELF: Invalid header\n");
    return -1;
  }

  // Validate program header table
  uint32_t phdr_size =
      hdr->ProgramHeaderTableEntryCount * hdr->ProgramHeaderTableEntrySize;
  if (hdr->ProgramHeaderTablePosition > elf_size ||
      hdr->ProgramHeaderTablePosition + phdr_size > elf_size) {
    debugf("ELF: Invalid program headers\n");
    return -1;
  }

  // Load segments
  uint8_t *phdrs = (uint8_t *)elf_data + hdr->ProgramHeaderTablePosition;

  for (uint32_t i = 0; i < hdr->ProgramHeaderTableEntryCount; i++) {
    ELFProgramHeader *ph =
        (ELFProgramHeader *)(phdrs + i * hdr->ProgramHeaderTableEntrySize);

    if (ph->Type != ELF_PROGRAM_TYPE_LOAD || ph->MemorySize == 0) {
      continue;
    }

    // Validate segment bounds
    if (ph->Offset > elf_size || ph->Offset + ph->FileSize > elf_size) {
      debugf("ELF: Segment %u out of bounds\n", i);
      return -1;
    }

    // Load segment
    uint32_t flags =
        PAGE_USER |
        ((ph->Flags & ELF_PROGRAM_FLAG_WRITABLE) ? PAGE_READWRITE : 0);
    void *file_data =
        (ph->FileSize > 0) ? ((uint8_t *)elf_data + ph->Offset) : NULL;

    if (load_segment(vm, ph->VirtualAddress, ph->MemorySize, file_data,
                     ph->FileSize, flags) != 0) {
      debugf("ELF: Failed to load segment %u\n", i);
      return -1;
    }
  }

  *entry = hdr->ProgramEntryPosition;
  return 0;
}
