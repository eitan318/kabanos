#include "elf.h"
#include "../fat/fat.h"
#include "../include/stdio.h"
#include "../include/string.h"
#include "../kmalloc.h"
#include "../memory_management/frame_allocator.h"
#include "../memory_management/paging.h"

void *elf_load(PageDirectory *page_dir, const char *filepath) {
  // Read entire ELF file into memory
  void *elf_data = NULL;
  uint32_t elf_size = 0;

  if (fat_read_file(filepath, &elf_data, &elf_size) != 0) {
    debugf("ELF: Failed to read file\n");
    return NULL;
  }

  // Validate ELF header
  ELFHeader *header = (ELFHeader *)elf_data;

  if (memcmp(header->Magic, ELF_MAGIC, 4) != 0) {
    debugf("ELF: Invalid magic number\n");
    kfree(elf_data);
    return NULL;
  }

  if (header->Bitness != ELF_BITNESS_32BIT) {
    debugf("ELF: Not a 32-bit ELF\n");
    kfree(elf_data);
    return NULL;
  }

  if (header->Endianness != ELF_ENDIANNESS_LITTLE) {
    debugf("ELF: Not little endian\n");
    kfree(elf_data);
    return NULL;
  }

  if (header->Type != ELF_TYPE_EXECUTABLE) {
    debugf("ELF: Not an executable\n");
    kfree(elf_data);
    return NULL;
  }

  if (header->InstructionSet != ELF_INSTRUCTION_SET_X86) {
    debugf("ELF: Not x86 instruction set\n");
    kfree(elf_data);
    return NULL;
  }

  void *entry_point = (void *)header->ProgramEntryPosition;

  // Process program headers
  uint8_t *prog_headers = (uint8_t *)elf_data + header->ProgramHeaderTablePosition;

  for (uint32_t i = 0; i < header->ProgramHeaderTableEntryCount; i++) {
    ELFProgramHeader *prog_hdr =
        (ELFProgramHeader *)(prog_headers + i * header->ProgramHeaderTableEntrySize);

    // Only load LOAD segments
    if (prog_hdr->Type != ELF_PROGRAM_TYPE_LOAD) {
      continue;
    }

    // Calculate number of pages needed
    uint32_t virt_start = prog_hdr->VirtualAddress & PAGE_FRAME_MASK;
    uint32_t virt_end = (prog_hdr->VirtualAddress + prog_hdr->MemorySize + PAGE_SIZE - 1) & PAGE_FRAME_MASK;
    uint32_t num_pages = (virt_end - virt_start) / PAGE_SIZE;

    // Determine page flags
    uint32_t page_flags = PTE_PRESENT | PTE_USER;
    if (prog_hdr->Flags & ELF_PROGRAM_FLAG_WRITABLE) {
      page_flags |= PTE_WRITE;
    }

    // Allocate and map pages
    for (uint32_t page = 0; page < num_pages; page++) {
      uint32_t virt_addr = virt_start + (page * PAGE_SIZE);

      // Allocate physical frame
      uint32_t phys_addr = frame_alloc();
      if (phys_addr == 0) {
        debugf("ELF: Failed to allocate frame\n");
        kfree(elf_data);
        return NULL;
      }

      // Map the page
      if (!paging_page_map(page_dir, virt_addr, phys_addr, page_flags)) {
        debugf("ELF: Failed to map page at 0x%x\n", virt_addr);
        frame_free(phys_addr);
        kfree(elf_data);
        return NULL;
      }

      // Zero the page first (important for BSS)
      memset((void *)phys_addr, 0, PAGE_SIZE);
    }

    // Copy file data to memory
    if (prog_hdr->FileSize > 0) {
      uint8_t *src = (uint8_t *)elf_data + prog_hdr->Offset;

      for (uint32_t offset = 0; offset < prog_hdr->FileSize; offset += PAGE_SIZE) {
        uint32_t virt_addr = prog_hdr->VirtualAddress + offset;
        uint32_t phys_addr = paging_physical_address_get(page_dir, virt_addr);

        if (phys_addr == 0) {
          debugf("ELF: Failed to get physical address\n");
          kfree(elf_data);
          return NULL;
        }

        uint32_t copy_size = PAGE_SIZE;
        if (offset + PAGE_SIZE > prog_hdr->FileSize) {
          copy_size = prog_hdr->FileSize - offset;
        }

        // Copy to physical address
        uint32_t page_offset = virt_addr & (PAGE_SIZE - 1);
        memcpy((void *)(phys_addr + page_offset), src + offset, copy_size);
      }
    }
  }

  kfree(elf_data);
  return entry_point;
}