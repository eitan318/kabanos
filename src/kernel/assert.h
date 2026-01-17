#pragma once

#include "arch/i686/panic.h" // for halt / cli / hlt
#include "include/stdio.h"

#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("ASSERTION FAILED: %s (%s:%d)", #cond, __FILE__, __LINE__);       \
      arch_panic_halt();                                                       \
    }                                                                          \
  } while (0)
