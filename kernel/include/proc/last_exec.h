/**
 * @file last_exec.h
 * @brief Last-executed-program record, exposed for the GDB scripts.
 */
#pragma once

#include <klib/stdint.h>

#define EXEC_TABLE_MAX 16

/** @brief Path and load address of an executed program. */
typedef struct {
  char path[64];       /**< Executable path. */
  uintptr_t load_base; /**< Lowest PT_LOAD vaddr of the image. */
} executed_proc_t;

/** @brief Read by scripts/gdb/load_user_syms.py to locate user symbols. */
extern executed_proc_t g_last_executed_proc_for_gdb;

void exec_table_add(const char *path, uintptr_t load_base);
