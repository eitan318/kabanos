#pragma once
#include "klib/stdint.h"

// On i686, addresses are 32-bit
typedef uint32_t arch_vaddr_t;
typedef uint32_t arch_paddr_t;

typedef struct arch_thread_t {
  void *kernel_esp; // On x86, we just need to save the stack pointer
} arch_thread_t;

// TODO: make definition hidden
typedef struct arch_vm_t {
  uint32_t *pd;         // Pointer to Page Directory
  arch_paddr_t pd_phys; // Physical address (to load into CR3)
} arch_vm_t;

// depend on isr_common code
typedef struct trap_frame {
  // Pushed manually in isr_common
  uint32_t gs, fs, es, ds;
  // Pushed by pusha
  uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
  // Pushed by the specific ISR stub
  uint32_t interrupt, error;
  // Pushed automatically by CPU
  uint32_t eip, cs, eflags, esp_user, ss_user;
} __attribute__((packed)) trap_frame_t;
