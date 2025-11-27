// Bootloader CPUID Information Collector

#include "cpu_info.h"

// Execute CPUID instruction
static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax,
                         uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
  asm volatile("cpuid"
               : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
               : "a"(leaf), "c"(subleaf));
}

// Collect all CPU information
void collect_cpu_info(CPUInfo *info) {
  uint32_t eax, ebx, ecx, edx;

  // Zero out structure
  for (int i = 0; i < sizeof(CPUInfo); i++) {
    ((uint8_t *)info)[i] = 0;
  }

  // CPUID.00H - Get vendor string and max basic leaf
  cpuid(0, 0, &eax, &ebx, &ecx, &edx);
  info->max_basic_cpuid = eax;

  // Store vendor string (EBX, EDX, ECX order)
  *((uint32_t *)&info->vendor[0]) = ebx;
  *((uint32_t *)&info->vendor[4]) = edx;
  *((uint32_t *)&info->vendor[8]) = ecx;
  info->vendor[12] = '\0';

  // CPUID.01H - Get processor info and feature bits
  if (info->max_basic_cpuid >= 1) {
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    // Parse processor signature
    info->stepping = eax & 0xF;
    info->model = (eax >> 4) & 0xF;
    info->family = (eax >> 8) & 0xF;
    info->processor_type = (eax >> 12) & 0x3;
    info->extended_model = (eax >> 16) & 0xF;
    info->extended_family = (eax >> 20) & 0xFF;

    // Parse EDX feature flags
    info->fpu = (edx >> 0) & 1;
    info->pse = (edx >> 3) & 1;
    info->pae = (edx >> 6) & 1;
    info->msr = (edx >> 5) & 1;
    info->apic = (edx >> 9) & 1;
    info->sep = (edx >> 11) & 1;
    info->mtrr = (edx >> 12) & 1;
    info->pge = (edx >> 13) & 1;
    info->cmov = (edx >> 15) & 1;
    info->pat = (edx >> 16) & 1;
    info->pse36 = (edx >> 17) & 1;
    info->mmx = (edx >> 23) & 1;
    info->fxsr = (edx >> 24) & 1;
    info->sse = (edx >> 25) & 1;
    info->sse2 = (edx >> 26) & 1;
    info->htt = (edx >> 28) & 1;

    // Parse ECX feature flags
    info->sse3 = (ecx >> 0) & 1;
    info->ssse3 = (ecx >> 9) & 1;
    info->sse4_1 = (ecx >> 19) & 1;
    info->sse4_2 = (ecx >> 20) & 1;
    info->x2apic = (ecx >> 21) & 1;
    info->aes = (ecx >> 25) & 1;
    info->xsave = (ecx >> 26) & 1;
    info->avx = (ecx >> 28) & 1;
    info->rdrand = (ecx >> 30) & 1;

    // Get logical processor count
    info->logical_processors = (ebx >> 16) & 0xFF;
    info->cache_line_size = ((ebx >> 8) & 0xFF) * 8;
  }

  // CPUID.07H - Extended features
  if (info->max_basic_cpuid >= 7) {
    cpuid(7, 0, &eax, &ebx, &ecx, &edx);

    info->fsgsbase = (ebx >> 0) & 1;
    info->bmi1 = (ebx >> 3) & 1;
    info->bmi2 = (ebx >> 8) & 1;
    info->avx2 = (ebx >> 5) & 1;
    info->smep = (ebx >> 7) & 1;
    info->smap = (ebx >> 20) & 1;
    info->avx512f = (ebx >> 16) & 1;
    info->rdseed = (ebx >> 18) & 1;
    info->sha = (ebx >> 29) & 1;
  }

  // CPUID.80000000H - Get max extended leaf
  cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
  info->max_extended_cpuid = eax;

  // CPUID.80000001H - Extended processor features
  if (info->max_extended_cpuid >= 0x80000001) {
    cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);

    info->syscall = (edx >> 11) & 1;
    info->nx = (edx >> 20) & 1;
    info->pdpe1gb = (edx >> 26) & 1;
    info->rdtscp = (edx >> 27) & 1;
    info->lm = (edx >> 29) & 1; // Long mode (64-bit)
  }

  // CPUID.80000006H - Cache information
  if (info->max_extended_cpuid >= 0x80000006) {
    cpuid(0x80000006, 0, &eax, &ebx, &ecx, &edx);

    info->l2_cache_size = (ecx >> 16) & 0xFFFF;         // KB
    info->l3_cache_size = ((edx >> 18) & 0x3FFF) * 512; // KB
  }

  // CPUID.80000008H - Address sizes
  if (info->max_extended_cpuid >= 0x80000008) {
    cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);

    info->phys_addr_bits = eax & 0xFF;
    info->virt_addr_bits = (eax >> 8) & 0xFF;
    info->cores_per_package = (ecx & 0xFF) + 1;
  }
}
