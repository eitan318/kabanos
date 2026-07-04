/**
 * @file msr.h
 * @brief Model-specific register (MSR) access.
 */
#include "klib/stdint.h"

/* SYSENTER MSRs */
#define MSR_IA32_SYSENTER_CS 0x174
#define MSR_IA32_SYSENTER_ESP 0x175
#define MSR_IA32_SYSENTER_EIP 0x176

/**
 * @brief Writes @p val to the MSR @p msr.
 *
 * MSRs are 64-bit, but the SYSENTER MSRs used on i686 only need the low
 * 32 bits, so the high half is always written as zero.
 */
static inline void wrmsr(uint32_t msr, uint32_t val) {
  uint32_t low = val;
  uint32_t high = 0;
  asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}
