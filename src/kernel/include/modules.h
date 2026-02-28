#pragma once
#include "klib/stddef.h"

typedef enum {
  MODULE_LOADING,
  MODULE_LOADED,
} module_state_t;

typedef struct module_t module_t;

typedef struct module_t {
  const char *name;      // Human readable name (e.g., "fat32")
  const char **required; // List of module names this depends on

  // Function pointers
  int (*init)(module_t *self);
  int (*fini)(void);

  // Multiboot / Payload data (Optional: only filled if it's an external file)
  void *data_start;
  size_t data_size;

  int state; // 0: Unloaded, 1: Loading, 2: Loaded

  module_t *next;
} module_t;

#define ITER_MODULE(name)                                                      \
  static module_t __mod_##name __attribute__((used, section(".modules")))

void modules_init_registry(module_t *dynamic_modules);
void modules_load();
