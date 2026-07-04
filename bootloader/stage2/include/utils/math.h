/**
 * @file math.h
 * @brief Min/max and alignment helpers.
 */
#pragma once
#include <stdint.h>

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

extern void __attribute__((cdecl))
div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotient_out,
         uint32_t *reminder_out);
