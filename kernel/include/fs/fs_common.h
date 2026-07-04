/**
 * @file fs_common.h
 * @brief Platform hooks that decouple filesystem drivers from the kernel.
 *
 * Filesystem code (e.g. myfs) calls back through this table for memory,
 * block I/O and logging, so the same driver can also be linked into host
 * tools like mkfs.
 */
#pragma once

#include <klib/stddef.h>
#include <klib/stdint.h>

/** @brief Environment services provided to filesystem drivers. */
typedef struct {
  void *(*alloc)(size_t size);
  void (*free)(void *ptr);

  /** @brief Reads one block at @p lba from @p dev into @p buf. */
  int (*read_block)(const void *dev, uint32_t lba, void *buf);
  /** @brief Writes one block at @p lba to @p dev from @p buf. */
  int (*write_block)(const void *dev, uint32_t lba, const void *buf);

  void (*log)(const char *fmt, ...);
} fs_platform_t;
