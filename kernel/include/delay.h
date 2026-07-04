/**
 * @file delay.h
 * @brief Busy-wait delay routines.
 */
#include "klib/stdint.h"

/**
 * @brief Busy-waits for approximately @p ns nanoseconds.
 *
 * @param ns Duration to wait, in nanoseconds.
 * @param cpu_loops_per_ns Calibration factor: spin-loop iterations per
 *        nanosecond on this CPU.
 */
void ndelay(uint64_t ns, uint64_t cpu_loops_per_ns);
