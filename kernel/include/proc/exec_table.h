#pragma once

#include "klib/stddef.h"
#include "klib/stdint.h"

#define EXEC_TABLE_MAX 16

typedef struct {
  char path[64];
  uintptr_t load_base; // lowest PT_LOAD vaddr
} exec_table_entry_t;

void exec_table_add(const char *path, uintptr_t load_base);
