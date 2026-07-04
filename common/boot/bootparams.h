/**
 * @file bootparams.h
 * @brief Multiboot2 structures shared between the bootloader and the kernel.
 */
#pragma once

#define MAX_MODULES 16
#define MAX_MEMORY_REGIONS 256

/** @brief Physical address where stage2 places the Multiboot2 info block. */
#define BOOT_PARAMS_PHYSICAL_ADDR 0x7A000

/** @brief Value passed in EAX to the kernel by a Multiboot2 bootloader. */
#define MULTIBOOT2_MAGIC 0x36d76289

/* Multiboot2 tag types */
#define MB2_TAG_END 0
#define MB2_TAG_CMDLINE 1
#define MB2_TAG_BOOT_LOADER_NAME 2
#define MB2_TAG_MODULE 3
#define MB2_TAG_MMAP 6

/* We use the compiler-defined types because neither s2lib/stdint.h nor
   klib/stdint.h can be included here: both include this file. */
#ifndef __UINT32_TYPE__
#error "Compiler does not support __UINT32_TYPE__ built-ins"
#else
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
#endif

/** @brief Common header of every Multiboot2 tag. */
typedef struct mb2_tag_t {
  uint32_t type;
  uint32_t size; /**< Size of the tag including this header. */
} __attribute__((packed)) mb2_tag_t;

/** @brief String tag (MB2_TAG_CMDLINE, MB2_TAG_BOOT_LOADER_NAME). */
typedef struct mb2_tag_string_t {
  mb2_tag_t tag;
  char string[];
} __attribute__((packed)) mb2_tag_string_t;

/** @brief Boot module tag (MB2_TAG_MODULE). */
typedef struct mb2_tag_module_t {
  mb2_tag_t tag;
  uint32_t mod_start; /**< Physical start address of the module. */
  uint32_t mod_end;   /**< Physical end address (exclusive). */
  char cmdline[];     /**< Module command line / name. */
} __attribute__((packed)) mb2_tag_module_t;

/** @brief One region in the memory map. */
typedef struct mb2_mmap_entry_t {
  uint64_t addr; /**< Start of the region. */
  uint64_t len;  /**< Length of the region in bytes. */
  uint32_t type; /**< 1 = usable RAM; other values are reserved regions. */
  uint32_t reserved;
} __attribute__((packed)) mb2_mmap_entry_t;

/** @brief Memory map tag (MB2_TAG_MMAP). */
typedef struct mb2_tag_mmap_t {
  mb2_tag_t tag;
  uint32_t entry_size;        /**< sizeof(mb2_mmap_entry_t). */
  uint32_t entry_version;     /**< Usually 0. */
  mb2_mmap_entry_t entries[]; /**< entry array; count derives from tag.size. */
} __attribute__((packed)) mb2_tag_mmap_t;

/** @brief Root Multiboot2 information block. */
typedef struct mb2_info_t {
  uint32_t total_size; /**< Total size of the block including all tags. */
  uint32_t reserved;
  uint8_t tags[]; /**< First tag; walk using each tag's size (8-aligned). */
} __attribute__((packed)) mb2_info_t;
