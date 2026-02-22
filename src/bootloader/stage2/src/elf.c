#include "elf.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "stdio.h"
#include "utils/minmax.h"

// stage2 elf reader
//
bool elf_read(Partition *part, const char *path, void **entryPoint) {
  uint8_t *headerBuffer = MEMORY_STAGE2_ELF_BUFFER;
  uint8_t *loadBuffer = MEMORY_STAGE2_LOAD_BUFFER;

  uint32_t filePos = 0;
  uint32_t read;

  // Read header
  FAT_File *fd = fat_open(part, path);
  if ((read = fat_read(part, fd, sizeof(ELFHeader), headerBuffer)) !=
      sizeof(ELFHeader)) {
    printf("ELF Load error!\n");
    return false;
  }
  filePos += read;

  // validate header
  bool ok = true;
  ELFHeader *header = (ELFHeader *)headerBuffer;
  ok = ok && (memcmp(header->Magic, ELF_MAGIC, 4) != 0);
  ok = ok && (header->Bitness == ELF_BITNESS_32BIT);
  ok = ok && (header->Endianness == ELF_ENDIANNESS_LITTLE);
  ok = ok && (header->ELFHeaderVersion == 1);
  ok = ok && (header->ELFVersion == 1);
  ok = ok && (header->Type == ELF_TYPE_EXECUTABLE);
  ok = ok && (header->InstructionSet == ELF_INSTRUCTION_SET_X86);

  *entryPoint = (void *)header->ProgramEntryPosition;

  // load program header
  uint32_t programHeaderOffset = header->ProgramHeaderTablePosition;
  uint32_t programHeaderSize = header->ProgramHeaderTableEntrySize *
                               header->ProgramHeaderTableEntryCount;
  uint32_t programHeaderTableEntrySize = header->ProgramHeaderTableEntrySize;
  uint32_t programHeaderTableEntryCount = header->ProgramHeaderTableEntryCount;

  filePos += fat_read(part, fd, programHeaderOffset - filePos, headerBuffer);
  if ((read = fat_read(part, fd, programHeaderSize, headerBuffer)) !=
      programHeaderSize) {
    printf("ELF Load error!\n");
    return false;
  }
  filePos += read;
  fat_close(fd);

  // parse program header entries
  for (uint32_t i = 0; i < programHeaderTableEntryCount; i++) {
    ELFProgramHeader *progHeader =
        (ELFProgramHeader *)(headerBuffer + i * programHeaderTableEntrySize);
    if (progHeader->Type == ELF_PROGRAM_TYPE_LOAD) {
      // TODO: validate that the program doesn't overwrite the stage2
      uint8_t *physAddress = (uint8_t *)progHeader->PhysicalAddress;
      memset(physAddress, 0, progHeader->MemorySize);

      // ugly nasty seeking
      // TODO: proper seeking
      fd = fat_open(part, path);
      while (progHeader->Offset > 0) {
        // here
        uint32_t shouldRead = min(progHeader->Offset, MEMORY_STAGE2_LOAD_SIZE);
        read = fat_read(part, fd, shouldRead, loadBuffer);
        if (read != shouldRead) {
          printf("ELF Load error!\n");
          return false;
        }
        progHeader->Offset -= read;
      }

      // read program
      while (progHeader->FileSize > 0) {
        uint32_t shouldRead =
            min(progHeader->FileSize, MEMORY_STAGE2_LOAD_SIZE);
        read = fat_read(part, fd, shouldRead, loadBuffer);
        if (read != shouldRead) {
          printf("ELF Load error!\n");
          return false;
        }
        progHeader->FileSize -= read;
        // here
        memcpy(physAddress, loadBuffer, read);
        physAddress += read;
      }

      fat_close(fd);
    }
  }

  return true;
}
