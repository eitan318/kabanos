// #pragma once
//
// #include <klib/stdint.h>
//
// #define EXEC_TABLE_MAX 16
//
// typedef struct {
//   char path[64];
//   uintptr_t load_base; // lowest PT_LOAD vaddr
// } executed_proc_t;
//
// // This symbol is visible to GDB
// extern exec_table_entry_t g_last_executed_proc_for_gdb;
//
// void exec_table_add(const char *path, uintptr_t load_base);
