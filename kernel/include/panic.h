/**
 * @file panic.h
 * @brief Kernel panic interface for unrecoverable errors.
 */
#include "hal.h"

/**
 * @brief Panics with the CPU state captured in a trap frame.
 *
 * Used by exception handlers to report the register state at the
 * point of the fault before halting.
 *
 * @param regs Register snapshot taken on kernel entry.
 */
void panic_from_regs(trap_frame_t *regs);

/**
 * @brief Prints a formatted error message and halts the system.
 *
 * @param fmt printf-style format string describing the failure.
 */
void __attribute__((noreturn)) panic(const char *fmt, ...);
