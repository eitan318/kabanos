#pragma once

#include "panic.h"
#include "stdio.h"

#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      panic_halt("ASSERTION FAILED: %s (%s:%d)", #cond, __FILE__, __LINE__);   \
    }                                                                          \
  } while (0)
