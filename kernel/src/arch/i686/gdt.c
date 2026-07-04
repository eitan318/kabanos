/**
 * @file gdt.c
 * @brief GDT construction: kernel/user segments plus per-core TSS descriptors.
 */
#include "arch/i686/gdt.h"
#include "arch/i686/tss.h"
#include "klib/stdint.h"
#include "klib/string.h"
#define TY_CODE 0x8 // Executable
#define TY_DATA 0x0
#define TY_READABLE 0x2
#define TY_WRITABLE 0x2
#define TY_ACCESSED 0x1

// Private indices — only visible in this file
typedef enum {
  GDT_NULL_SEGMENT = 0,
  GDT_KERNEL_CS,
  GDT_KERNEL_DS,
  GDT_USER_CS,
  GDT_USER_DS,
  GDT_DEFAULT_ENTRIES_COUNT,
} gdt_entry_index_t;

// Public constants computed from private indices
const gdt_selector_t i686_GDT_NULL_SEL = (GDT_NULL_SEGMENT << 3);
const gdt_selector_t i686_GDT_KERNEL_CS_SEL = (GDT_KERNEL_CS << 3);
const gdt_selector_t i686_GDT_KERNEL_DS_SEL = (GDT_KERNEL_DS << 3);
const gdt_selector_t i686_GDT_USER_CS_SEL = (GDT_USER_CS << 3) | 3;
const gdt_selector_t i686_GDT_USER_DS_SEL = (GDT_USER_DS << 3) | 3;

typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

// Helper function to set a GDT entry dynamically
static void gdt_set_entry(gdt_entry_t *entry, uint32_t base, uint32_t limit,
                          uint8_t type, uint8_t s, uint8_t dpl, uint8_t p,
                          uint8_t l, uint8_t d, uint8_t g) {
  entry->limit_low = limit & 0xFFFF;
  entry->base_low = base & 0xFFFF;
  entry->base_middle = (base >> 16) & 0xFF;

  entry->access = (type & 0xF)       // Type
                  | ((s & 1) << 4)   // S bit
                  | ((dpl & 3) << 5) // DPL
                  | ((p & 1) << 7);  // Present

  entry->granularity = ((limit >> 16) & 0x0F) // Limit high 4 bits
                       | ((l & 1) << 5)       // L
                       | ((d & 1) << 6)       // D/B
                       | ((g & 1) << 7);      // Granularity

  entry->base_high = (base >> 24) & 0xFF;
}

static gdt_entry_t entries[MAX_CORES + GDT_DEFAULT_ENTRIES_COUNT];

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) gdt_descriptor_t;

int i686_gdt_init() {

  gdt_set_entry(&entries[GDT_NULL_SEGMENT], 0, 0x0, 0, 0, 0, 0, 0, 0, 0);
  gdt_set_entry(&entries[GDT_KERNEL_CS], 0, 0xFFFFF, TY_CODE | TY_READABLE, 1,
                0, 1, 0, 1, 1);
  gdt_set_entry(&entries[GDT_KERNEL_DS], 0, 0xFFFFF, TY_DATA | TY_WRITABLE, 1,
                0, 1, 0, 1, 1);
  gdt_set_entry(&entries[GDT_USER_CS], 0, 0xFFFFF, TY_CODE | TY_READABLE, 1, 3,
                1, 0, 1, 1);
  gdt_set_entry(&entries[GDT_USER_DS], 0, 0xFFFFF, TY_DATA | TY_WRITABLE, 1, 3,
                1, 0, 1, 1);

  gdt_descriptor_t gdt_descriptor;
  int num_cores = 1;
  int num_gdt_entries = num_cores + GDT_DEFAULT_ENTRIES_COUNT;

  for (int i = 0; i < num_cores; ++i) {
    tss_entry_set(tss_entry_get(i));
    gdt_set_entry(&entries[i + GDT_DEFAULT_ENTRIES_COUNT],
                  (uint32_t)tss_entry_get(i), sizeof(tss_entry_t) - 1,
                  TY_CODE | TY_ACCESSED, 0, 3, 1, 0, 0, 1);
  }

  gdt_descriptor.base = (uint32_t)&entries[0];
  gdt_descriptor.limit = sizeof(gdt_entry_t) * num_gdt_entries - 1;

  __asm volatile("lgdt %0;"
                 "mov  %1, %%ax;"
                 "mov  %%ax, %%ds;"
                 "mov  %%ax, %%es;"
                 "mov  %%ax, %%fs;"
                 "mov  %%ax, %%gs;"
                 "ljmp %2, $1f;"
                 "1:"
                 :
                 : "m"(gdt_descriptor),
                   "r"(i686_GDT_KERNEL_DS_SEL), // AX for data segments
                   "i"(i686_GDT_KERNEL_CS_SEL)  // immediate for ljmp
                 : "ax");

  // Load the TSS for CPU 0
  __asm volatile("ltr %0" : : "r"((uint16_t)(GDT_DEFAULT_ENTRIES_COUNT << 3)));

  return 0;
}
