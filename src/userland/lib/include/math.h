#pragma once
#include <stdbool.h>
#include <stdint.h>

extern void __attribute__((cdecl))
div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotient_out,
         uint32_t *reminder_out);
