#pragma once
#include <stdint.h>

// On i686, addresses are 32-bit
typedef uint32_t arch_vaddr_t;
typedef uint32_t arch_paddr_t;

typedef struct arch_thread_t {
  void *esp; // On x86, we just need to save the stack pointer
} thread_context_t;

typedef struct arch_vm_t {
  uint32_t *pd;         // Pointer to Page Directory
  arch_paddr_t pd_phys; // Physical address (to load into CR3)
} vm_context_t;

typedef struct regs {
  // in the reverse order they are pushed:
  uint32_t ds; // data segment pushed by us
  uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // pusha
  uint32_t interrupt,
      error; // we push interrupt, error is pushed automatically (or our dummy)
  uint32_t eip, cs, eflags, esp_user, ss_user; // pushed automatically by CPU
} __attribute__((packed)) i686_regs_t;
