/** @file syscall.h */

#pragma once
#include "klib/stdint.h"

/** @brief Captured syscall state from assembly stub */
typedef struct {
  void *context;    ///< CPU register state at time of interrupt
  uint32_t num;     ///< System call ID (EAX)
  uint32_t args[6]; ///< Arguments from registers (EBX, ECX, EDX, ESI, EDI, EBP)
} syscall_info_t;

/** @brief Route syscall to the appropriate kernel handler */
long syscall_dispatch(syscall_info_t f);

/** @brief Setup interrupt gate or MSRs for syscall entry */
void syscall_init();
