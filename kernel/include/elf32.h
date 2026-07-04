/**
 * @file elf32.h
 * @brief ELF32 structures and executable loading.
 */
#pragma once
#include "arch/types.h"
#include "hal.h"
#include "klib/stdbool.h"
#include "klib/stdint.h"
#include "mm/vmspace.h"

#define ELF_MAGIC                                                              \
  "\x7F"                                                                       \
  "ELF"

/** @brief ELF32 file header. */
typedef struct {
  uint8_t magic[4];   /**< Must equal ELF_MAGIC. */
  uint8_t wordsize;   /**< @ref elf_wordsize */
  uint8_t endianness; /**< @ref elf_endianness */
  uint8_t version;
  uint8_t abi;
  uint8_t _padding[8];
  uint16_t type; /**< @ref elf_type */
  uint16_t instruction_set;
  uint32_t elf_version;
  uint32_t program_entry_pos; /**< Entry point virtual address. */
  uint32_t phdr_table_pos;    /**< File offset of the program header table. */
  uint32_t shdr_table_pos;    /**< File offset of the section header table. */
  uint32_t flags;
  uint16_t header_size;
  uint16_t phdr_table_entry_size;
  uint16_t phdr_table_entry_count;
  uint16_t shdr_table_entry_size;
  uint16_t shdr_table_entry_count;
  uint16_t section_names_index; /**< Section index of the .shstrtab table. */
} __attribute__((packed)) elf32_header_t;

/** @brief ELF32 section header. */
typedef struct {
  uint32_t name; /**< Offset into the section-name string table. */
  uint32_t type;
  uint32_t flags;
  uint32_t addr;   /**< Virtual address at execution. */
  uint32_t offset; /**< File offset of the section data. */
  uint32_t size;
  uint32_t link;
  uint32_t info;
  uint32_t addralign;
  uint32_t entsize;
} elf32_sec_hdr;

/** @brief ELF32 program (segment) header. */
typedef struct {
  uint32_t type; /**< @ref ELFProgramType */
  uint32_t off;  /**< File offset of the segment data. */
  uint32_t vaddr;
  uint32_t paddr;
  uint32_t file_size; /**< Bytes stored in the file. */
  uint32_t mem_size;  /**< Bytes occupied in memory (>= file_size; the
                           remainder is zero-filled, e.g. .bss). */
  uint32_t flags;     /**< @ref ELFProgramFlags */
  uint32_t align;
} elf32_prog_hdr;

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

enum ELFProgramType {
  ELF_PROGRAM_TYPE_NULL = 0,
  ELF_PROGRAM_TYPE_LOAD = 1,
  ELF_PROGRAM_TYPE_DYNAMIC = 2,
  ELF_PROGRAM_TYPE_INTERP = 3,
  ELF_PROGRAM_TYPE_NOTE = 4,
  ELF_PROGRAM_TYPE_SHLIB = 5,
  ELF_PROGRAM_TYPE_PHDR = 6,
  ELF_PROGRAM_TYPE_TLS = 7,
};

enum ELFProgramFlags {
  ELF_PROGRAM_FLAG_EXECUTABLE = 0x1,
  ELF_PROGRAM_FLAG_WRITABLE = 0x2,
  ELF_PROGRAM_FLAG_READABLE = 0x4,
};

/**
 * @brief Validates an in-memory ELF image and maps its PT_LOAD segments
 *        into @p vmspace.
 * @param vmspace Destination address space.
 * @param elf_data ELF image already loaded into kernel memory.
 * @param elf_size Size of the image in bytes.
 * @param entry [out] Program entry point.
 * @param text_base [out] Virtual address of the .text section.
 * @return 0 on success, negative errno on failure.
 */
int elf32_load(vmspace_t *vmspace, void *elf_data, uint32_t elf_size,
               uintptr_t *entry, uintptr_t *text_base);

/** @brief Returns the virtual address of the .text section, or 0 if absent. */
uintptr_t elf32_find_text_section(void *elf_data, elf32_header_t *hdr);
