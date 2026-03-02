#pragma once

#include <klib/stddef.h>
#include <klib/stdint.h>

typedef struct {
  void *(*alloc)(size_t size);
  void (*free)(void *ptr);

  int (*read_block)(void *dev, uint32_t lba, void *buf);
  int (*write_block)(void *dev, uint32_t lba, const void *buf);

  void (*log)(const char *fmt, ...);
} fs_platform_t;
