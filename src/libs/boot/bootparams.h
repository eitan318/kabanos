#pragma once
#include <stdbool.h>
#include <stdint.h>

#define MAX_MODULES 16
#define MAX_MEMORY_REGIONS 256

#define BOOT_PARAMS_PHYSICAL_ADDR 0x7A000

#define MULTIBOOT2_MAGIC 0x36d76289

#define MB2_TAG_END 0
#define MB2_TAG_CMDLINE 1
#define MB2_TAG_BOOT_LOADER_NAME 2
#define MB2_TAG_MODULE 3
#define MB2_TAG_MMAP 6

typedef struct mb2_tag_t {
  uint32_t type;
  uint32_t size; // size of tag including header
} __attribute__((packed)) mb2_tag_t;

// Command line tag
typedef struct mb2_tag_string_t {
  mb2_tag_t tag;
  char string[];
} __attribute__((packed)) mb2_tag_string_t;

// Module tag
typedef struct mb2_tag_module_t {
  mb2_tag_t tag;
  uint32_t mod_start;
  uint32_t mod_end;
  char cmdline[];
} __attribute__((packed)) mb2_tag_module_t;

// Memory map entry
typedef struct mb2_mmap_entry_t {
  uint64_t addr; // start of region
  uint64_t len;  // length of region
  uint32_t type;
  uint32_t reserved;
} __attribute__((packed)) mb2_mmap_entry_t;

// Memory map tag
typedef struct mb2_tag_mmap_t {
  mb2_tag_t tag;
  uint32_t entry_size;        // sizeof(mb2_mmap_entry)
  uint32_t entry_version;     // usually 0
  mb2_mmap_entry_t entries[]; // flexible array
} __attribute__((packed)) mb2_tag_mmap_t;

// Full mb2 info pointer
typedef struct mb2_info_t {
  uint32_t total_size;
  uint32_t reserved;
  uint8_t tags[]; // pointer to first tag (walk tags using tag->size)
} __attribute__((packed)) mb2_info_t;
