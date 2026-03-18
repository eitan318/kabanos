#pragma once
#include "klib/stdint.h"

typedef struct tss_entry {
  uint32_t prev;
  uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
  uint32_t cr3, eip, eflags;
  uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
  uint32_t es, cs, ss, ds, fs, gs;
  uint32_t ldt;
  uint16_t trap, iomap_base;
} __attribute__((packed)) tss_entry_t;

tss_entry_t *tss_entry_get(int cpu_id);
void tss_entry_set(tss_entry_t *e);
