#pragma once
#include <stdint.h>

typedef struct {
  uint32_t num;
  uint32_t args[6];
} syscall_frame_t;

long syscall_dispatch(const syscall_frame_t *frame);

void syscall_init();
