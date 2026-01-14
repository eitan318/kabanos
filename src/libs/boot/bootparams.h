#pragma once
#include <stdbool.h>
#include <stdint.h>

#define MAX_MODULES 16
#define MAX_MEMORY_REGIONS 256

#define BOOT_PARAMS_PHYSICAL_ADDR 0x7A000
typedef struct DiskParams {
  uint8_t hdds_count;
  uint8_t drive_id;
  uint16_t cylinders;
  uint16_t sectors;
  uint16_t heads;
  bool lba_support;
} DiskParams;

enum MBRPartitionEntryFlag {
  BOOTABLE = 0x80,
  NON_BOOTABLE = 0x00,
};

enum MBRPartitionEntryType {
  FAT12 = 0x01,
  FAT16 = 0x04,       // optional
  FAT32_CHS = 0x0B,   // FAT32 with CHS addressing
  FAT32_LBA = 0x0C,   // FAT32 LBA variant
  EXTENDED_0F = 0x0F, // extended partition
  EXTENDED_05 = 0x05, // extended partition (CHS)
  EXTENDED_85 = 0x85, // extended partition (Linux)
};

typedef struct __attribute__((packed)) {
  uint8_t boot_flag;      // 0x80 = bootable, 0x00 = non-bootable
  uint8_t chs_start[3];   // CHS address of first sector (ignored by LBA)
  uint8_t partition_type; // 0x01=FAT12, 0x0B/F=FAT32, etc
  uint8_t chs_end[3];
  uint32_t lba_start;
  uint32_t total_sectors;
} MBRPartitionEntry;

typedef struct {
  int entries_count;
  MBRPartitionEntry *partition_entries;
} PartitionTable;

// Structure to pass CPU info to kernel
typedef struct {
  // Basic CPUID info
  uint32_t max_basic_cpuid;
  uint32_t max_extended_cpuid;
  char vendor[13];

  // CPU features (CPUID.01H)
  uint32_t stepping : 4;
  uint32_t model : 4;
  uint32_t family : 4;
  uint32_t processor_type : 2;
  uint32_t extended_model : 4;
  uint32_t extended_family : 8;

  // Feature flags from EDX (CPUID.01H)
  bool fpu;   // x87 FPU
  bool pse;   // Page Size Extension
  bool pae;   // Physical Address Extension
  bool msr;   // Model Specific Registers
  bool apic;  // APIC on chip
  bool sep;   // SYSENTER/SYSEXIT
  bool mtrr;  // Memory Type Range Registers
  bool pge;   // Page Global Enable
  bool cmov;  // CMOV instruction
  bool pat;   // Page Attribute Table
  bool pse36; // 36-bit PSE
  bool mmx;   // MMX Technology
  bool fxsr;  // FXSAVE/FXRSTOR
  bool sse;   // SSE
  bool sse2;  // SSE2
  bool htt;   // Hyper-Threading Technology

  // Feature flags from ECX (CPUID.01H)
  bool sse3;   // SSE3
  bool ssse3;  // SSSE3
  bool sse4_1; // SSE4.1
  bool sse4_2; // SSE4.2
  bool x2apic; // x2APIC
  bool aes;    // AES instruction set
  bool xsave;  // XSAVE/XRSTOR
  bool avx;    // AVX
  bool rdrand; // RDRAND

  // Extended features (CPUID.07H)
  bool fsgsbase; // FSGSBASE instructions
  bool bmi1;     // Bit Manipulation Instruction Set 1
  bool bmi2;     // Bit Manipulation Instruction Set 2
  bool avx2;     // AVX2
  bool smep;     // Supervisor Mode Execution Prevention
  bool smap;     // Supervisor Mode Access Prevention
  bool avx512f;  // AVX-512 Foundation
  bool rdseed;   // RDSEED instruction
  bool sha;      // SHA extensions

  // Extended CPUID info
  bool syscall; // SYSCALL/SYSRET
  bool nx;      // No-Execute bit
  bool pdpe1gb; // 1GB pages
  bool rdtscp;  // RDTSCP instruction
  bool lm;      // Long mode (64-bit)

  // Cache info
  uint32_t cache_line_size;
  uint32_t l2_cache_size; // KB
  uint32_t l3_cache_size; // KB

  // Processor counts
  uint32_t logical_processors;
  uint32_t cores_per_package;

  // Address sizes
  uint32_t phys_addr_bits;
  uint32_t virt_addr_bits;

} CPUInfo;

enum E820MemoryBlockType {
  E820_USABLE = 1,
  E820_RESERVED = 2,
  E820_ACPI_RECLAIMABLE = 3,
  E820_ACPI_NVS = 4,
  E820_BAD_MEMORY = 5,
};

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
