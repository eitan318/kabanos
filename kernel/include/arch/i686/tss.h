/**
 * @file tss.h
 * @brief Task State Segment access.
 *
 * The TSS is only used to supply the kernel stack (esp0/ss0) when the CPU
 * transitions from user mode to kernel mode; hardware task switching is
 * not used.
 */
#pragma once
#include "klib/stdint.h"

/** @brief Hardware-defined 32-bit TSS layout. */
typedef struct tss_entry {
  uint32_t prev;
  uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
  uint32_t cr3, eip, eflags;
  uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
  uint32_t es, cs, ss, ds, fs, gs;
  uint32_t ldt;
  uint16_t trap, iomap_base;
} __attribute__((packed)) tss_entry_t;

/** @brief Returns the TSS of the given CPU. */
tss_entry_t *tss_entry_get(int cpu_id);

/** @brief Loads the task register with the selector for @p e. */
void tss_entry_set(tss_entry_t *e);
