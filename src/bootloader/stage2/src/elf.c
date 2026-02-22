#include "elf.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "stdio.h"
#include "utils/minmax.h"

static bool fat_seek(Partition *part, FAT_File *fd, uint32_t offset,
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

bool elf_read(Partition *part, const char *path, void **entryPoint) {
  uint8_t *headerBuffer = MEMORY_STAGE2_ELF_BUFFER;
  uint8_t *loadBuffer = MEMORY_STAGE2_LOAD_BUFFER;
  uint32_t read;

  FAT_File *fd = fat_open(part, path);

  // 1. Read ELF header
  uint32_t filePos = 0;
  if ((read = fat_read(part, fd, sizeof(ELFHeader), headerBuffer)) !=
      sizeof(ELFHeader)) {
    printf("ELF: failed to read header\n");
    fat_close(fd);
    return false;
  }
  filePos += read;

  // 2. Validate header
  ELFHeader *header = (ELFHeader *)headerBuffer;
  if (memcmp(header->Magic, ELF_MAGIC, 4) != 0 ||
      header->Bitness != ELF_BITNESS_32BIT ||
      header->Endianness != ELF_ENDIANNESS_LITTLE ||
      header->ELFHeaderVersion != 1 || header->ELFVersion != 1 ||
      header->Type != ELF_TYPE_EXECUTABLE ||
      header->InstructionSet != ELF_INSTRUCTION_SET_X86) {
    printf("ELF: invalid header\n");
    fat_close(fd);
    return false;
  }
  *entryPoint = (void *)header->ProgramEntryPosition;

  // 3. Read program header table
  uint32_t phOffset = header->ProgramHeaderTablePosition;
  uint32_t phEntSize = header->ProgramHeaderTableEntrySize;
  uint32_t phEntCount = header->ProgramHeaderTableEntryCount;
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
    ELFProgramHeader *ph = (ELFProgramHeader *)(headerBuffer + i * phEntSize);

    if (ph->Type != ELF_PROGRAM_TYPE_LOAD)
      continue;

    uint8_t *physAddress = (uint8_t *)ph->PhysicalAddress;
    uint32_t fileSize = ph->FileSize;
    uint32_t memSize = ph->MemorySize;
    uint32_t segOffset = ph->Offset;

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
