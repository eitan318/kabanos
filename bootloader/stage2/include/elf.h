#pragma once
// stage2 elf reader
#include "mbr.h"
#include <stdbool.h>
#include <stdint.h>

#define ELF_MAGIC                                                              \
  ("\x7F"                                                                      \
   "ELF")

typedef struct {
  uint8_t magic[4];
  uint8_t wordsize;   // 1 = 32 bit, 2 = 64 bit
  uint8_t endianness; // 1 = little endian, 2 = big endian
  uint8_t version;
  uint8_t abi;
  uint8_t _padding[8];
  uint16_t type; // 1 = relocatable, 2 = executable, 3 = shared, 4 = core
  uint16_t instruction_set;
  uint32_t elf_version;
  uint32_t program_entry_pos;
  uint32_t phdr_table_pos;
  uint32_t shdr_table_pos;
  uint32_t flags;
  uint16_t header_size;
  uint16_t phdr_table_entry_size;
  uint16_t phdr_table_entry_count;
  uint16_t shdr_table_entry_size;
  uint16_t shdr_table_entry_count;
  uint16_t section_names_index;

} __attribute__((packed)) elf32_header_t;

enum elf_wordsize {
  ELF_BITNESS_32BIT = 1,
  ELF_BITNESS_64BIT = 2,
};

enum elf_endianness {
  ELF_ENDIANNESS_LITTLE = 1,
  ELF_ENDIANNESS_BIG = 2,
};

enum elf_instruction_set {
  ELF_INSTRUCTION_SET_NONE = 0,
  ELF_INSTRUCTION_SET_X86 = 3,
  ELF_INSTRUCTION_SET_ARM = 0x28,
  ELF_INSTRUCTION_SET_X64 = 0x3E,
  ELF_INSTRUCTION_SET_ARM64 = 0xB7,
  ELF_INSTRUCTION_SET_RISCV = 0xF3,
};

enum elf_type {
  ELF_TYPE_RELOCATABLE = 1,
  ELF_TYPE_EXECUTABLE = 2,
  ELF_TYPE_SHARED = 3,
  ELF_TYPE_CORE = 4,
};

typedef struct {
  uint32_t type;
  uint32_t off;
  uint32_t vaddr;
  uint32_t paddr;
  uint32_t file_size;
  uint32_t mem_size;
  uint32_t flags;
  uint32_t align;

} elf32_prog_hdr;

enum ELFProgramType {
  // Program header table entry unused.
  ELF_PROGRAM_TYPE_NULL = 0,

  // Loadable segment.
  ELF_PROGRAM_TYPE_LOAD = 1,

  // Dynamic linking information.
  ELF_PROGRAM_TYPE_DYNAMIC = 2,

  // Interpreter information.
  ELF_PROGRAM_TYPE_INTERP = 3,

  // Auxiliary information.
  ELF_PROGRAM_TYPE_NOTE = 4,

  // Reserved
  ELF_PROGRAM_TYPE_SHLIB = 5,

  // Segment containing program header table itself.
  ELF_PROGRAM_TYPE_PHDR = 6,

  // Thread-Local Storage template.
  ELF_PROGRAM_TYPE_TLS = 7,

  // Reserved inclusive range. Operating system specific.
  ELF_PROGRAM_TYPE_LOOS = 0x60000000,
  ELF_PROGRAM_TYPE_HIOS = 0x6FFFFFFF,

  // Reserved inclusive range. Processor specific.
  ELF_PROGRAM_TYPE_LOPROC = 0x70000000,
  ELF_PROGRAM_TYPE_HIPROC = 0x7FFFFFFF,
};

bool elf_read(partition_t *part, const char *path, void **entryPoint);
