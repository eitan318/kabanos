#include "proc/exec_table.h"
#include <klib/stddef.h>
#include <klib/string.h>

exec_table_entry_t g_exec_table[EXEC_TABLE_MAX];
int g_exec_table_count = 0;

void exec_table_add(const char *path, uintptr_t load_base) {
  if (g_exec_table_count >= EXEC_TABLE_MAX)
    return;
  g_exec_table_count = 0;
  exec_table_entry_t *e = &g_exec_table[0];
  // 1. Zero out the entire struct first
  memset(e, 0, sizeof(exec_table_entry_t));
  strncpy(e->path, path, sizeof(e->path) - 1);
  e->path[sizeof(e->path) - 1] = '\0';
  e->load_base = load_base;
}
