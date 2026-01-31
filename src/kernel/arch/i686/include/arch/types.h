#pragma once
#include <stdint.h>

// On i686, addresses are 32-bit
typedef uint32_t arch_vaddr_t;
typedef uint32_t arch_paddr_t;

typedef struct arch_thread_t arch_thread_t;
typedef struct arch_vm_t arch_vm_t;
typedef struct arch_regs arch_regs;
