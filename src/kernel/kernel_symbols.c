#include "kernel_symbols.h"

const char *lookup_symbol(uintptr_t addr) {
  const char *result = "??";

  for (unsigned i = 0; i < symbol_count; i++) {
    if (symbols[i].addr > addr)
      break;
    result = symbols[i].name;
  }

  return result;
}
