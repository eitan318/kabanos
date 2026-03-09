#pragma once
#include "klib/stdint.h"
typedef uint64_t time_t;

typedef struct {
  time_t tv_sec; // Seconds
  long tv_nsec;  // Nanoseconds
} timespec_t;
