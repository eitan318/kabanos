#pragma once
#include <stdint.h>

#define MAX_CORES 20

typedef unsigned short gdt_selector_t;
extern const gdt_selector_t i686_GDT_NULL_SEL;
extern const gdt_selector_t i686_GDT_KERNEL_CS_SEL;
extern const gdt_selector_t i686_GDT_KERNEL_DS_SEL;
extern const gdt_selector_t i686_GDT_USER_CS_SEL;
extern const gdt_selector_t i686_GDT_USER_DS_SEL;

// Initialize GDT
int i686_gdt_init(void);
