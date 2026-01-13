// #include "print_boot_info.h"
// #include "boot/bootparams.h"
// #include "include/stdio.h"
//
// void print_boot_params(BootParams boot_params) {
//   print_partition_table(boot_params.partition_table);
//   print_memory_map(boot_params.memory_map);
//   print_disk_params(&boot_params.disk_params);
//   print_cpu_info(&boot_params.cpu_info);
//   print_cmdline(boot_params.cmdline_buffer, boot_params.cmdline_size);
// }
//
// void print_partition_table(PartitionTable partition_table) {
//   debugf("MBR Partition Table:\n"
//          "Idx | Boot | Type |   LBA Start  | Total Sectors\n"
//          "-----------------------------------------------\n");
//
//   for (int i = 0; i < partition_table.entries_count; i++) {
//     MBRPartitionEntry p = partition_table.partition_entries[i];
//     debugf("%d |  0x%X | 0x%X | %u | %u\n", i, p.boot_flag, p.partition_type,
//            p.lba_start, p.total_sectors);
//   }
// }
//
// void print_memory_map(MemoryMap memory_map) {
//   debugf("\nMemory map:\n"
//          "Idx | Base  |  Length    | Type | ACPI\n"
//          "-----------------------------------------------------------------\n");
//
//   for (int i = 0; i < memory_map.region_count; i++) {
//     const MemoryRegion *e = &memory_map.regions[i];
//
//     debugf("%x | %llx | %llx | %x | %x\n", i, e->base, e->length, e->type,
//            e->acpi_flag);
//   }
// }
//
// void print_disk_params(DiskParams *disk_params) {
//   debugf("\nDisk Parameters:\n"
//          "====================\n"
//          "Cylinders    : %u\n"
//          "Heads        : %u\n"
//          "Sectors/Track: %u\n"
//          "Drive ID     : %X\n"
//          "HDD Count    : %u\n"
//          "LBA Support  : %s\n",
//          disk_params->cylinders, disk_params->heads, disk_params->sectors,
//          disk_params->drive_id, disk_params->hdds_count,
//          disk_params->lba_support ? "Yes" : "No");
// }
//
// void print_cpu_info(const CPUInfo *cpu) {
//   debugf("\nCPU Info:\n");
//   debugf("Vendor        : %s\n", cpu->vendor);
//   debugf("Max Basic CPUID : 0x%X\n", cpu->max_basic_cpuid);
//   debugf("Max Extended CPUID : 0x%X\n", cpu->max_extended_cpuid);
//
//   debugf("\n-- CPU Version --\n");
//   debugf("Stepping      : %u\n", cpu->stepping);
//   debugf("Model         : %u\n", cpu->model);
//   debugf("Family        : %u\n", cpu->family);
//   debugf("Processor Type: %u\n", cpu->processor_type);
//   debugf("Extended Model: %u\n", cpu->extended_model);
//   debugf("Extended Family: %u\n", cpu->extended_family);
//
//   debugf("\n-- Features (EDX) --\n");
//   debugf("FPU   : %s\n", cpu->fpu ? "Yes" : "No");
//   debugf("PSE   : %s\n", cpu->pse ? "Yes" : "No");
//   debugf("PAE   : %s\n", cpu->pae ? "Yes" : "No");
//   debugf("MSR   : %s\n", cpu->msr ? "Yes" : "No");
//   debugf("APIC  : %s\n", cpu->apic ? "Yes" : "No");
//   debugf("SEP   : %s\n", cpu->sep ? "Yes" : "No");
//   debugf("MTRR  : %s\n", cpu->mtrr ? "Yes" : "No");
//   debugf("PGE   : %s\n", cpu->pge ? "Yes" : "No");
//   debugf("CMOV  : %s\n", cpu->cmov ? "Yes" : "No");
//   debugf("PAT   : %s\n", cpu->pat ? "Yes" : "No");
//   debugf("PSE36 : %s\n", cpu->pse36 ? "Yes" : "No");
//   debugf("MMX   : %s\n", cpu->mmx ? "Yes" : "No");
//   debugf("FXSR  : %s\n", cpu->fxsr ? "Yes" : "No");
//   debugf("SSE   : %s\n", cpu->sse ? "Yes" : "No");
//   debugf("SSE2  : %s\n", cpu->sse2 ? "Yes" : "No");
//
//   debugf("\n-- Features (ECX) --\n");
//   debugf("SSE3   : %s\n", cpu->sse3 ? "Yes" : "No");
//   debugf("SSSE3  : %s\n", cpu->ssse3 ? "Yes" : "No");
//   debugf("SSE4.1 : %s\n", cpu->sse4_1 ? "Yes" : "No");
//   debugf("SSE4.2 : %s\n", cpu->sse4_2 ? "Yes" : "No");
//   debugf("X2APIC : %s\n", cpu->x2apic ? "Yes" : "No");
//   debugf("AES    : %s\n", cpu->aes ? "Yes" : "No");
//   debugf("XSAVE  : %s\n", cpu->xsave ? "Yes" : "No");
//   debugf("AVX    : %s\n", cpu->avx ? "Yes" : "No");
//   debugf("RDRAND : %s\n", cpu->rdrand ? "Yes" : "No");
//
//   debugf("\n-- Extended Features (CPUID.07H) --\n");
//   debugf("FSGSBASE : %s\n", cpu->fsgsbase ? "Yes" : "No");
//   debugf("BMI1     : %s\n", cpu->bmi1 ? "Yes" : "No");
//   debugf("BMI2     : %s\n", cpu->bmi2 ? "Yes" : "No");
//   debugf("AVX2     : %s\n", cpu->avx2 ? "Yes" : "No");
//   debugf("SMEP     : %s\n", cpu->smep ? "Yes" : "No");
//   debugf("SMAP     : %s\n", cpu->smap ? "Yes" : "No");
//   debugf("AVX-512F : %s\n", cpu->avx512f ? "Yes" : "No");
//   debugf("RDSEED   : %s\n", cpu->rdseed ? "Yes" : "No");
//   debugf("SHA      : %s\n", cpu->sha ? "Yes" : "No");
//
//   debugf("\n-- Extended CPUID Info --\n");
//   debugf("SYSCALL : %s\n", cpu->syscall ? "Yes" : "No");
//   debugf("NX      : %s\n", cpu->nx ? "Yes" : "No");
//   debugf("PDPE1GB : %s\n", cpu->pdpe1gb ? "Yes" : "No");
//   debugf("RDTSCP  : %s\n", cpu->rdtscp ? "Yes" : "No");
//   debugf("Long Mode (LM) : %s\n", cpu->lm ? "Yes" : "No");
//
//   debugf("\n-- Cache Info --\n");
//   debugf("Cache Line Size : %u bytes\n", cpu->cache_line_size);
//   debugf("L2 Cache Size   : %u KB\n", cpu->l2_cache_size);
//   debugf("L3 Cache Size   : %u KB\n", cpu->l3_cache_size);
//
//   debugf("\n-- Processor Counts --\n");
//   debugf("Logical Processors  : %u\n", cpu->logical_processors);
//   debugf("Cores per Package  : %u\n", cpu->cores_per_package);
//
//   debugf("\n-- Address Sizes --\n");
//   debugf("Physical Address Bits : %u\n", cpu->phys_addr_bits);
//   debugf("Virtual Address Bits  : %u\n", cpu->virt_addr_bits);
// }
//
// void print_cmdline(char *cmdline, const int cmdline_size) {
//   debugf("\nCmdline: %s\n", cmdline);
// }
