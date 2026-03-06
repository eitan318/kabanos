#pragma once
#include "klib/stdint.h"

// This corresponds to syscall entry
typedef struct {
  void *context;
  uint32_t num;
  uint32_t args[6];
} syscall_info_t;

long syscall_dispatch(syscall_info_t f);

void syscall_init();
