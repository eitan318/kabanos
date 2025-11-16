#pragma once
#include <stdbool.h>
#include <stdint.h>

#define MAX_MODULES 16

typedef struct DiskParams {
  uint8_t hdds_count;
  uint8_t drive_id;
  uint16_t cylinders;
  uint16_t sectors;
  uint16_t heads;
  bool lba_support;
} DiskParams;

typedef struct __attribute__((packed)) {
  uint8_t boot_flag;      // 0x80 = bootable, 0x00 = non-bootable
  uint8_t chs_start[3];   // CHS address of first sector (ignored by LBA)
  uint8_t partition_type; // 0x01=FAT12, 0x0B/F=FAT32, etc
  uint8_t chs_end[3];
  uint32_t lba_start;
  uint32_t total_sectors;
} MBRPartitionEntry;

typedef struct {
  uint64_t base, length;
  uint32_t type, acpi_flag, reserved1, reserved2;
} __attribute__((packed)) MemoryRegion;

typedef struct {
  int region_count;
  MemoryRegion *regions;
} MemoryMap;

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

typedef struct {
  void *start;
  void *end;
  char *path;
} Module;

typedef struct {
  const MBRPartitionEntry *partition_table;
  int partitions_count;
  DiskParams disk_params;
  MemoryMap memory_map;
  CPUInfo *cpu_info;
  char *cmdline_buffer;
  int cmdline_size;
  void *kernel_start;
  void *kernel_end;
  void *initrd_start;
  void *initrd_end;
  Module modules[MAX_MODULES];
  int module_count;
} BootParams;
