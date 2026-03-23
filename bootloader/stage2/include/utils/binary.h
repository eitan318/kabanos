#pragma once
#include "stdint.h"

#define FLAG_SET(buf, flag_mask) buf |= (flag_mask)
#define FLAG_UNSET(buf, flag_mask) buf &= ~(flag_mask)

// Helper to convert VBE far pointer to physical address
static inline void *far_ptr_to_phys(uint16_t ptr[2]) {
  // ptr[0] is offset, ptr[1] is segment
  return (void *)(((uint32_t)ptr[1] << 4) + ptr[0]);
}
