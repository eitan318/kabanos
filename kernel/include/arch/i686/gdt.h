/**
 * @file gdt.h
 * @brief Global Descriptor Table setup and segment selectors.
 */
#pragma once
#include "klib/stdint.h"

#define MAX_CORES 20

typedef unsigned short gdt_selector_t;

/* Segment selectors as laid out by i686_gdt_init(). */
extern const gdt_selector_t i686_GDT_NULL_SEL;
extern const gdt_selector_t i686_GDT_KERNEL_CS_SEL;
extern const gdt_selector_t i686_GDT_KERNEL_DS_SEL;
extern const gdt_selector_t i686_GDT_USER_CS_SEL;
extern const gdt_selector_t i686_GDT_USER_DS_SEL;

/** @brief Builds the GDT (kernel/user segments plus per-core TSS entries)
 *         and loads it. */
int i686_gdt_init(void);
