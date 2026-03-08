#pragma once
#include "klib/stddef.h"

typedef enum {
  MODULE_LOADING,
  MODULE_LOADED,
} module_state_t;

typedef struct module_t module_t;

typedef struct module_t {
  const char *name;
  const char **required_modules_names;

  int (*init)(module_t *self);
  int (*fini)(void);

  void *data_start;
  size_t data_size;

  module_state_t state;

  module_t *next;
} module_t;

#define ITER_MODULE(name)                                                      \
  static module_t __mod_##name __attribute__((used, section(".modules")))

void modules_init_registry(module_t *dynamic_modules);
void modules_load();
