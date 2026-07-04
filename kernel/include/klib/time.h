/**
 * @file time.h
 * @brief Kernel time types.
 */
#pragma once
#include "klib/stdint.h"
typedef uint64_t time_t;

/** @brief A point in time with nanosecond resolution. */
typedef struct {
  time_t tv_sec; /**< Seconds. */
  long tv_nsec;  /**< Nanoseconds. */
} timespec_t;
