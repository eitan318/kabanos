/**
 * @file exec_table.h
 * @brief Record of executed programs and their load addresses.
 *
 * Consumed by the GDB helper scripts to load user-space symbols at the
 * right base address.
 */
#pragma once

#include "klib/stddef.h"
#include "klib/stdint.h"

#define EXEC_TABLE_MAX 16

/** @brief One executed program. */
typedef struct {
  char path[64];       /**< Executable path. */
  uintptr_t load_base; /**< Lowest PT_LOAD vaddr of the image. */
} exec_table_entry_t;

/** @brief Records that @p path was loaded at @p load_base. */
void exec_table_add(const char *path, uintptr_t load_base);
