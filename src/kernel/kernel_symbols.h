#pragma once
#include <stdint.h>

extern unsigned symbol_count;

typedef struct {
  uintptr_t addr;
  const char *name;
} symbol_t;

extern symbol_t symbols[];

const char *lookup_symbol(uintptr_t addr);
