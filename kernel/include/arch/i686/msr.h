#include "klib/stdint.h"

#define MSR_IA32_SYSENTER_CS 0x174
#define MSR_IA32_SYSENTER_ESP 0x175
#define MSR_IA32_SYSENTER_EIP 0x176

static inline void wrmsr(uint32_t msr, uint32_t val) {
  uint32_t low = val; // MSRs are 64-bit, but for i686
  uint32_t high = 0;  // sysenter, we usually only need the low 32
  asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}
