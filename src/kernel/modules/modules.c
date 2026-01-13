#include "modules.h"
#include "include/stdio.h"
#include "include/string.h"

// Load a specific module
int module_load(KernelBootInfo *kbi, const char *name) {
  for (uint32_t i = 0; i < kbi->module_count; i++) {
    // Check if module name matches (compare basename)
    const char *basename = strrchr(kbi->modules[i].cmdline, '/');
    if (!basename)
      basename = kbi->modules[i].cmdline;
    else
      basename++;

    if (strcmp(basename, name) == 0) {
      if (kbi->modules[i].loaded) {
        debugf("Module %s already loaded\n", name);
        return 0;
      }

      debugf("Loading module: %s\n", name);

      // Module loading logic depends on your module format
      // Example: if modules are ELF files, parse and load them
      // For now, just mark as loaded

      kbi->modules[i].loaded = 1;
      debugf("Module %s loaded successfully\n", name);
      return 0;
    }
  }

  debugf("Module %s not found\n", name);
  return -1;
}

// Get module data by name
void *module_get_data(KernelBootInfo *kbi, const char *name,
                      uint32_t *size_out) {
  for (uint32_t i = 0; i < kbi->module_count; i++) {
    const char *basename = strrchr(kbi->modules[i].cmdline, '/');
    if (!basename)
      basename = kbi->modules[i].cmdline;
    else
      basename++;

    if (strcmp(basename, name) == 0) {
      if (size_out) {
        *size_out = kbi->modules[i].size;
      }
      return kbi->modules[i].start;
    }
  }
  return NULL;
}
