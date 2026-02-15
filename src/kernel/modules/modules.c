#include "modules.h"
#include "kernel_boot_info.h"
#include "stdio.h"
#include "string.h"

static module_t *module_registry;
static int registry_count = 0;

extern module_t _modules_start[];
extern module_t _modules_end[];

void modules_init_registry(module_t *dynamic_modules) {
  // Add all static modules to the list first
  size_t static_count = _modules_end - _modules_start;
  for (size_t i = 0; i < static_count; i++) {
    module_t *static_mod = &_modules_start[i];

    // Push to front of the registry list
    static_mod->next = module_registry;
    module_registry = static_mod;
  }

  // Attach the dynamic list to the end of the current registry
  if (module_registry == NULL) {
    module_registry = dynamic_modules;
  } else {
    module_t *last = module_registry;
    while (last->next != NULL) {
      last = last->next;
    }
    last->next = dynamic_modules;
  }
}

module_t *find_module_by_name(const char *name) {
  module_t *mod = module_registry;
  while (mod != NULL) {
    if (!strcmp(mod->name, name)) {
      return mod;
    }
    mod = mod->next;
  }
  return NULL;
}

void module_load(module_t *mod) {
  if (mod->state == MODULE_LOADED)
    return;

  mod->state = MODULE_LOADING;

  if (mod->required) {
    for (int i = 0; mod->required[i] != NULL; i++) {
      module_t *dep = find_module_by_name(mod->required[i]);
      if (dep)
        module_load(dep);
    }
  }

  if (mod->init) {
    debugf("Initing module: %s\n", mod->name);
    mod->init(mod);
  }

  mod->state = MODULE_LOADED;
}

void modules_load() {
  module_t *mod = module_registry;
  while (mod != NULL) {
    module_load(mod);
    mod = mod->next;
  }
}
