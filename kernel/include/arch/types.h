/**
 * @file types.h
 * @brief Architecture-specific (i686) core types shared with generic code.
 */
#pragma once
#include "klib/stdint.h"

/* On i686, addresses are 32-bit */
typedef uint32_t arch_vaddr_t;
typedef uint32_t arch_paddr_t;

/** @brief Per-thread CPU state saved across context switches. */
typedef struct arch_thread_t {
  void *kernel_esp; /**< Saved kernel stack pointer; the rest of the context
                         lives on the kernel stack itself. */
} arch_thread_t;

/** @brief Architecture handle for a virtual address space. */
// TODO: make definition hidden
typedef struct arch_vmspace_t {
  uint32_t *pd;         /**< Page directory (virtual address). */
  arch_paddr_t pd_phys; /**< Physical address of the PD, for loading CR3. */
} arch_vm_t;

/**
 * @brief Register snapshot pushed on kernel entry.
 *
 * Layout must match the push order in the isr_common assembly stub.
 */
typedef struct trap_frame {
  /* Pushed manually in isr_common */
  uint32_t gs, fs, es, ds;
  /* Pushed by pusha */
  uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
  /* Pushed by the specific ISR stub */
  uint32_t interrupt, error;
  /* Pushed automatically by the CPU; esp_user/ss_user only on a
     privilege-level change */
  uint32_t eip, cs, eflags, esp_user, ss_user;
} __attribute__((packed)) trap_frame_t;
