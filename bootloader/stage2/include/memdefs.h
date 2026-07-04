/**
 * @file memdefs.h
 * @brief Fixed physical memory layout used by stage2.
 */
#pragma once

#define INITRD_LOAD_ADDR ((void *)0x200000)
#define MODULE_LOAD_ADDR ((void *)0x300000)

#define MEMORY_FAT_ADDR ((void *)0x20000)
#define MEMORY_FAT_SIZE 0x00010000
#define MEMORY_STAGE2_ELF_BUFFER ((void *)0x30000)
#define MEMORY_STAGE2_LOAD_BUFFER ((void *)0x40000)
#define MEMORY_STAGE2_LOAD_SIZE 0x10000 // 64 KB temporary buffer
#define MEMORY_HEAP_START 0x50000
#define MEMORY_HEAP_SIZE 0x20000
