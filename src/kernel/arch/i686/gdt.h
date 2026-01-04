#pragma once

#define i686_GDT_KERNEL_CODE_SEGMENT 0x08
#define i686_GDT_KERNEL_DATA_SEGMENT 0x10
#define i686_GDT_USER_CODE_SEGMENT (0x18 | 3)
#define i686_GDT_USER_DATA_SEGMENT (0x20 | 3)

void i686_gdt_init();
