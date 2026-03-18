#include "arch/i686/tss.h"
#include "arch/i686/gdt.h"
#include "klib/string.h"

static tss_entry_t tss_entries[MAX_CORES];

void tss_entry_set(tss_entry_t *e) {
  memset((uint8_t *)e, 0, sizeof(tss_entry_t));
  e->ss0 = e->ss = e->ds = e->es = e->fs = e->gs = i686_GDT_KERNEL_DS_SEL;
  e->cs = i686_GDT_KERNEL_CS_SEL;
}

tss_entry_t *tss_entry_get(int cpu_id) { return &tss_entries[cpu_id]; }
