#include "gdt.h"
#include <stdint.h>

// Enums for flags
typedef enum {
  GDT_ACCESS_SEG_DATA = 0 << 3,
  GDT_ACCESS_SEG_CODE = 1 << 3,
  GDT_ACCESS_CODE_WRITABLE = 1 << 1,
  GDT_ACCESS_DATA_READABLE = 1 << 1,
  GDT_ACCESS_CODE_CONFORMING = 1 << 2,
  GDT_ACCESS_DATA_DIRECTION_DOWN = 1 << 2,
  GDT_ACCESS_DATA_DIRECTION_NORMAL = 0 << 2,
  GDT_ACCESS_SEG_SYSTEM = 0 << 4,
  GDT_ACCESS_SEG_USER = 1 << 4,
  GDT_ACCESS_RING0 = 0 << 5,
  GDT_ACCESS_RING1 = 1 << 5,
  GDT_ACCESS_RING2 = 2 << 5,
  GDT_ACCESS_RING3 = 3 << 5,
  GDT_ACCESS_PRESENT = 1 << 7,
} GDTAccess;

typedef enum {
  GDT_FLAGS_64b = 1 << 5,
  GDT_FLAGS_32b = 1 << 6,
  GDT_FLAGS_16b = 0 << 6,
  GDT_FLAGS_GRAN_1B = 0 << 7,
  GDT_FLAGS_GRAN_4KB = 1 << 7,
} GDTFlags;

#define GDT_ENTRY(limit, base, access, flags)                                  \
  {                                                                            \
    (uint16_t)((limit)&0xffff), (uint16_t)((base)&0xffff),                     \
        (uint8_t)(((base) >> 16) & 0xff), (uint8_t)((access)&0xff),            \
        (uint8_t)((((limit) >> 16) & 0x0f) | ((flags)&0xf0)),                  \
        (uint8_t)(((base) >> 24) & 0xff)                                       \
  }

typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed)) GDTEntry;

GDTEntry gdt[] = {
    // NULL Descriptor
    GDT_ENTRY(0, 0, 0, 0),
    // Code segment
    GDT_ENTRY(0xfffff, 0,
              GDT_ACCESS_SEG_CODE | GDT_ACCESS_CODE_WRITABLE |
                  GDT_ACCESS_SEG_USER | GDT_ACCESS_PRESENT | GDT_ACCESS_RING0,
              GDT_FLAGS_32b | GDT_FLAGS_GRAN_4KB),
    // Data segment
    GDT_ENTRY(0xfffff, 0,
              GDT_ACCESS_SEG_DATA | GDT_ACCESS_DATA_DIRECTION_NORMAL |
                  GDT_ACCESS_DATA_READABLE | GDT_ACCESS_SEG_USER |
                  GDT_ACCESS_PRESENT | GDT_ACCESS_RING0,
              GDT_FLAGS_32b | GDT_FLAGS_GRAN_4KB),
};

typedef struct {
  uint16_t size;
  GDTEntry *offset;
} __attribute__((packed)) GDTDescriptor;

static GDTDescriptor gdt_descriptor = {sizeof(gdt) - 1, gdt};

// Functions
void __attribute__((cdecl))
i686_gdt_load(GDTDescriptor *gdt_descriptor, uint16_t code_segment,
              uint16_t data_segment);

void i686_gdt_init() {
  i686_gdt_load(&gdt_descriptor, i686_GDT_KERNEL_CODE_SEGMENT,
                i686_GDT_KERNEL_DATA_SEGMENT);
}
