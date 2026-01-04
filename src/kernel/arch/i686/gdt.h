#pragma once
#include <stdint.h>

typedef unsigned short GDTSelector;
extern const GDTSelector i686_GDT_NULL_SEL;
extern const GDTSelector i686_GDT_KERNEL_CS_SEL;
extern const GDTSelector i686_GDT_KERNEL_DS_SEL;
extern const GDTSelector i686_GDT_USER_CS_SEL;
extern const GDTSelector i686_GDT_USER_DS_SEL;

typedef struct {
  uint32_t prev;
  uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
  uint32_t cr3, eip, eflags;
  uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
  uint32_t es, cs, ss, ds, fs, gs;
  uint32_t ldt;
  uint16_t trap, iomap_base;
} __attribute__((packed)) TSSEntry;

// Initialize GDT
int i686_gdt_init(void);
TSSEntry *tss_entry_get(int cpu_id);
