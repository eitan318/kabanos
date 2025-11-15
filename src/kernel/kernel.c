#include "arch/i686/vga_text.h"
#include "include/memory.h"
#include "include/stdio.h"
#include <stdbool.h>
#include <stdint.h>

extern uint8_t __bss_start;
extern uint8_t __end;

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
  uint64_t base;
  uint64_t length;
  uint32_t type;
  uint32_t acpi_flags;
  uint32_t reserved1;
  uint32_t reserved2;
} __attribute__((packed)) E820Entry;

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
  const MBRPartitionEntry *partition_table;
  int partitions_count;
  DiskParams disk_params;
  const E820Entry *memory_map;
  int memory_map_entry_count;
  CPUInfo *cpu_info;
  char *cmdline_buffer;
  int cmdline_size;
} BootInfo;

void print_partition_table(const MBRPartitionEntry *partition_table,
                           const int partitions_count) {
  debugf("MBR Partition Table:\n"
         "Idx | Boot | Type |   LBA Start  | Total Sectors\n"
         "-----------------------------------------------\n");

  for (int i = 0; i < partitions_count; i++) {
    MBRPartitionEntry p = partition_table[i];
    debugf("%d |  0x%X | 0x%X | %u | %u\n", i, p.boot_flag, p.partition_type,
           p.lba_start, p.total_sectors);
  }
}

void print_memory_map(const E820Entry *memory_map,
                      const int memory_map_entry_count) {
  debugf("\nMemory map:\n"
         "Idx | Base  |  Length    | Type | ACPI\n"
         "-----------------------------------------------------------------\n");

  for (int i = 0; i < memory_map_entry_count; i++) {
    const E820Entry *e = &memory_map[i];

    debugf("%d | %llu | %llu | %u | %u\n", i, e->base, e->length, e->type,
           e->acpi_flags);
  }
}

void print_disk_params(DiskParams *disk_params) {
  debugf("\nDisk Parameters:\n"
         "====================\n"
         "Cylinders    : %u\n"
         "Heads        : %u\n"
         "Sectors/Track: %u\n"
         "Drive ID     : %X\n"
         "HDD Count    : %u\n"
         "LBA Support  : %s\n",
         disk_params->cylinders, disk_params->heads, disk_params->sectors,
         disk_params->drive_id, disk_params->hdds_count,
         disk_params->lba_support ? "Yes" : "No");
}

void print_cpu_info(const CPUInfo *cpu) {
  debugf("\nCPU Info:\n");
  debugf("Vendor        : %s\n", cpu->vendor);
  debugf("Max Basic CPUID : 0x%X\n", cpu->max_basic_cpuid);
  debugf("Max Extended CPUID : 0x%X\n", cpu->max_extended_cpuid);

  debugf("\n-- CPU Version --\n");
  debugf("Stepping      : %u\n", cpu->stepping);
  debugf("Model         : %u\n", cpu->model);
  debugf("Family        : %u\n", cpu->family);
  debugf("Processor Type: %u\n", cpu->processor_type);
  debugf("Extended Model: %u\n", cpu->extended_model);
  debugf("Extended Family: %u\n", cpu->extended_family);

  debugf("\n-- Features (EDX) --\n");
  debugf("FPU   : %s\n", cpu->fpu ? "Yes" : "No");
  debugf("PSE   : %s\n", cpu->pse ? "Yes" : "No");
  debugf("PAE   : %s\n", cpu->pae ? "Yes" : "No");
  debugf("MSR   : %s\n", cpu->msr ? "Yes" : "No");
  debugf("APIC  : %s\n", cpu->apic ? "Yes" : "No");
  debugf("SEP   : %s\n", cpu->sep ? "Yes" : "No");
  debugf("MTRR  : %s\n", cpu->mtrr ? "Yes" : "No");
  debugf("PGE   : %s\n", cpu->pge ? "Yes" : "No");
  debugf("CMOV  : %s\n", cpu->cmov ? "Yes" : "No");
  debugf("PAT   : %s\n", cpu->pat ? "Yes" : "No");
  debugf("PSE36 : %s\n", cpu->pse36 ? "Yes" : "No");
  debugf("MMX   : %s\n", cpu->mmx ? "Yes" : "No");
  debugf("FXSR  : %s\n", cpu->fxsr ? "Yes" : "No");
  debugf("SSE   : %s\n", cpu->sse ? "Yes" : "No");
  debugf("SSE2  : %s\n", cpu->sse2 ? "Yes" : "No");

  debugf("\n-- Features (ECX) --\n");
  debugf("SSE3   : %s\n", cpu->sse3 ? "Yes" : "No");
  debugf("SSSE3  : %s\n", cpu->ssse3 ? "Yes" : "No");
  debugf("SSE4.1 : %s\n", cpu->sse4_1 ? "Yes" : "No");
  debugf("SSE4.2 : %s\n", cpu->sse4_2 ? "Yes" : "No");
  debugf("X2APIC : %s\n", cpu->x2apic ? "Yes" : "No");
  debugf("AES    : %s\n", cpu->aes ? "Yes" : "No");
  debugf("XSAVE  : %s\n", cpu->xsave ? "Yes" : "No");
  debugf("AVX    : %s\n", cpu->avx ? "Yes" : "No");
  debugf("RDRAND : %s\n", cpu->rdrand ? "Yes" : "No");

  debugf("\n-- Extended Features (CPUID.07H) --\n");
  debugf("FSGSBASE : %s\n", cpu->fsgsbase ? "Yes" : "No");
  debugf("BMI1     : %s\n", cpu->bmi1 ? "Yes" : "No");
  debugf("BMI2     : %s\n", cpu->bmi2 ? "Yes" : "No");
  debugf("AVX2     : %s\n", cpu->avx2 ? "Yes" : "No");
  debugf("SMEP     : %s\n", cpu->smep ? "Yes" : "No");
  debugf("SMAP     : %s\n", cpu->smap ? "Yes" : "No");
  debugf("AVX-512F : %s\n", cpu->avx512f ? "Yes" : "No");
  debugf("RDSEED   : %s\n", cpu->rdseed ? "Yes" : "No");
  debugf("SHA      : %s\n", cpu->sha ? "Yes" : "No");

  debugf("\n-- Extended CPUID Info --\n");
  debugf("SYSCALL : %s\n", cpu->syscall ? "Yes" : "No");
  debugf("NX      : %s\n", cpu->nx ? "Yes" : "No");
  debugf("PDPE1GB : %s\n", cpu->pdpe1gb ? "Yes" : "No");
  debugf("RDTSCP  : %s\n", cpu->rdtscp ? "Yes" : "No");
  debugf("Long Mode (LM) : %s\n", cpu->lm ? "Yes" : "No");

  debugf("\n-- Cache Info --\n");
  debugf("Cache Line Size : %u bytes\n", cpu->cache_line_size);
  debugf("L2 Cache Size   : %u KB\n", cpu->l2_cache_size);
  debugf("L3 Cache Size   : %u KB\n", cpu->l3_cache_size);

  debugf("\n-- Processor Counts --\n");
  debugf("Logical Processors  : %u\n", cpu->logical_processors);
  debugf("Cores per Package  : %u\n", cpu->cores_per_package);

  debugf("\n-- Address Sizes --\n");
  debugf("Physical Address Bits : %u\n", cpu->phys_addr_bits);
  debugf("Virtual Address Bits  : %u\n", cpu->virt_addr_bits);
}

void print_cmdline(char *cmdline, const int cmdline_size) {
  debugf("\nCmdline: %s", cmdline);
}

void load_modules() {}

void __attribute__((section(".entry"))) start(BootInfo *bootInfo) {
  memset(&__bss_start, 0, (&__end) - (&__bss_start));
  vga_clrscr();
  vga_setcursor(0, 0);

  print_partition_table(bootInfo->partition_table, bootInfo->partitions_count);
  print_memory_map(bootInfo->memory_map, bootInfo->memory_map_entry_count);
  print_disk_params(&bootInfo->disk_params);
  print_cpu_info(bootInfo->cpu_info);
  print_cmdline(bootInfo->cmdline_buffer, bootInfo->cmdline_size);

  load_modules();

  for (;;) {
  }
}
