#pragma once
#include "hal.h"
// depend on isr_common code
typedef struct arch_regs {
  // Pushed manually in isr_common
  uint32_t gs, fs, es, ds;
  // Pushed by pusha
  uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
  // Pushed by the specific ISR stub
  uint32_t interrupt, error;
  // Pushed automatically by CPU
  uint32_t eip, cs, eflags, esp_user, ss_user;
} __attribute__((packed)) arch_regs;
