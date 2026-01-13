#pragma once

#pragma once

#define INITRD_LOAD_ADDR ((void *)0x200000)
#define MODULE_LOAD_ADDR ((void *)0x300000)

#define MEMORY_STAGE2_ELF_BUFFER ((void *)0x30000)
#define MEMORY_STAGE2_LOAD_BUFFER ((void *)0x40000)
#define MEMORY_STAGE2_LOAD_SIZE 0x10000 // 64 KB temporary buffer

// from boot params: #define BOOT_PARAMS_PHYSICAL_ADDR 0x00008000 // 32KB mark
// (safe location)
