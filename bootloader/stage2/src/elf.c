#include "elf.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "s2lib/stdio.h"
#include "utils/minmax.h"

static bool fat_seek(partition_t *part, fat_file *fd, uint32_t offset,
                     uint8_t *scratch, uint32_t scratchSize) {
  while (offset > 0) {
    uint32_t toRead = min(offset, scratchSize);
    uint32_t read = fat_read(part, fd, toRead, scratch);
    if (read != toRead)
      return false;
    offset -= read;
  }
  return true;
}

bool elf_read(partition_t *part, const char *path, void **entryPoint) {
  uint8_t *headerBuffer = MEMORY_STAGE2_ELF_BUFFER;
  uint8_t *loadBuffer = MEMORY_STAGE2_LOAD_BUFFER;
  uint32_t read;

  fat_file *fd = fat_open(part, path);

  // 1. Read ELF header
  uint32_t filePos = 0;
  if ((read = fat_read(part, fd, sizeof(elf32_header_t), headerBuffer)) !=
      sizeof(elf32_header_t)) {
    printf("ELF: failed to read header\n");
    fat_close(fd);
    return false;
  }
  filePos += read;

  // 2. Validate header
  elf32_header_t *header = (elf32_header_t *)headerBuffer;
  if (memcmp(header->magic, ELF_MAGIC, 4) != 0 ||
      header->wordsize != ELF_BITNESS_32BIT ||
      header->endianness != ELF_ENDIANNESS_LITTLE || header->version != 1 ||
      header->elf_version != 1 || header->type != ELF_TYPE_EXECUTABLE ||
      header->instruction_set != ELF_INSTRUCTION_SET_X86) {
    printf("ELF: invalid header\n");
    fat_close(fd);
    return false;
  }
  *entryPoint = (void *)header->program_entry_pos;

  // 3. Read program header table
  uint32_t phOffset = header->phdr_table_pos;
  uint32_t phEntSize = header->phdr_table_entry_size;
  uint32_t phEntCount = header->phdr_table_entry_count;
  uint32_t phTotalSize = phEntSize * phEntCount;

  // Seek to program header table (still using the same fd)
  if (!fat_seek(part, fd, phOffset - filePos, loadBuffer,
                MEMORY_STAGE2_LOAD_SIZE)) {
    printf("ELF: seek to phdrs failed\n");
    fat_close(fd);
    return false;
  }
  filePos = phOffset;

  if ((read = fat_read(part, fd, phTotalSize, headerBuffer)) != phTotalSize) {
    printf("ELF: failed to read program headers\n");
    fat_close(fd);
    return false;
  }
  filePos += read;

  // 4. Load each PT_LOAD segment — still the same fd, just seek within it
  for (uint32_t i = 0; i < phEntCount; i++) {
    elf32_prog_hdr *ph = (elf32_prog_hdr *)(headerBuffer + i * phEntSize);

    if (ph->type != ELF_PROGRAM_TYPE_LOAD)
      continue;

    uint8_t *physAddress = (uint8_t *)ph->paddr;
    uint32_t fileSize = ph->file_size;
    uint32_t memSize = ph->mem_size;
    uint32_t segOffset = ph->off;

    if (segOffset < filePos) {
      printf("ELF: backward seek not supported\n");
      fat_close(fd);
      return false;
    }

    if (!fat_seek(part, fd, segOffset - filePos, loadBuffer,
                  MEMORY_STAGE2_LOAD_SIZE)) {
      printf("ELF: seek to segment %u failed\n", i);
      fat_close(fd);
      return false;
    }
    filePos = segOffset;

    // Read file data directly into physAddress (no intermediate copy)
    uint32_t remaining = fileSize;
    uint8_t *dest = physAddress;
    while (remaining > 0) {
      uint32_t chunk = min(remaining, MEMORY_STAGE2_LOAD_SIZE);
      read = fat_read(part, fd, chunk, dest); // <-- direct load, no memcpy
      if (read != chunk) {
        printf("ELF: segment read error\n");
        fat_close(fd);
        return false;
      }
      dest += read;
      remaining -= read;
    }
    filePos += fileSize;

    // // Zero only the BSS (MemorySize - FileSize), not the whole segment
    // if (memSize > fileSize) {
    //   memset(physAddress + fileSize, 0, memSize - fileSize);
    // }
  }

  fat_close(fd);
  return true;
}
