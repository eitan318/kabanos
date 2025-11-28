#include "modules.h"
#include "include/stdio.h"
#include "include/string.h"

// Global initrd data

static Module g_modules[16];
static uint32_t g_module_count = 0;

// Initialize modules from boot parameters
void modules_init(BootParams *params) {
  g_module_count = params->module_count;

  debugf("Initializing %u modules\n", g_module_count);

  for (uint32_t i = 0; i < g_module_count; i++) {
    g_modules[i].start = params->modules[i].start;
    g_modules[i].end = params->modules[i].end;
    g_modules[i].path = params->modules[i].path;
    g_modules[i].size = params->modules[i].size;
    g_modules[i].loaded = 0;
    g_modules[i].module_data = NULL;

    debugf("  Module %u: %s (0x%p - 0x%p, %u bytes)\n", i, g_modules[i].path,
           g_modules[i].start, g_modules[i].end, g_modules[i].size);
  }
}

// Load a specific module
int module_load(const char *name) {
  for (uint32_t i = 0; i < g_module_count; i++) {
    // Check if module name matches (compare basename)
    const char *basename = strrchr(g_modules[i].path, '/');
    if (!basename)
      basename = g_modules[i].path;
    else
      basename++;

    if (strcmp(basename, name) == 0) {
      if (g_modules[i].loaded) {
        debugf("Module %s already loaded\n", name);
        return 0;
      }

      debugf("Loading module: %s\n", name);

      // Module loading logic depends on your module format
      // Example: if modules are ELF files, parse and load them
      // For now, just mark as loaded

      g_modules[i].loaded = 1;
      debugf("Module %s loaded successfully\n", name);
      return 0;
    }
  }

  debugf("Module %s not found\n", name);
  return -1;
}

// Get module data by name
void *module_get_data(const char *name, uint32_t *size_out) {
  for (uint32_t i = 0; i < g_module_count; i++) {
    const char *basename = strrchr(g_modules[i].path, '/');
    if (!basename)
      basename = g_modules[i].path;
    else
      basename++;

    if (strcmp(basename, name) == 0) {
      if (size_out) {
        *size_out = g_modules[i].size;
      }
      return g_modules[i].start;
    }
  }
  return NULL;
}
