/**
 * @file assert.h
 * @brief Kernel assertion macro.
 */
#pragma once

#include "klib/stdio.h"
#include "panic.h"

/**
 * @brief Panics with file and line information if @p cond is false.
 *
 * Always compiled in; there is no NDEBUG variant.
 */
#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      panic("ASSERTION FAILED: %s (%s:%d)", #cond, __FILE__, __LINE__);        \
    }                                                                          \
  } while (0)
