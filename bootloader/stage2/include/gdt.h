/**
 * @file gdt.h
 * @brief Flat GDT for protected mode.
 */
#pragma once

#define i686_GDT_CODE_SEGMENT 0x08
#define i686_GDT_DATA_SEGMENT 0x10

void i686_gdt_init();
