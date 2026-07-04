/**
 * @file stdbool.h
 * @brief Boolean type for freestanding kernel code.
 *
 * Note: bool is a 32-bit integer here, not C99 _Bool, so sizeof(bool) == 4.
 */
#pragma once
#include "klib/stdint.h"

#define bool uint32_t
#define true 1
#define false 0
#define __bool_true_false_are_defined 1
