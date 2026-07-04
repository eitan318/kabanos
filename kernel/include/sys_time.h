/**
 * @file sys_time.h
 * @brief Time-of-day syscall.
 */
#include "klib/time.h"
#include "sched/timer.h"

/**
 * @brief Returns the current wall-clock time.
 * @param tv [out] Receives the current wall-clock time.
 * @param tz Unused; kept for POSIX signature compatibility.
 * @return 0 on success, -1 if @p tv is NULL.
 */
long sys_gettimeofday(timespec_t *tv, void *tz);
