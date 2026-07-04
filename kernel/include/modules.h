/**
 * @file modules.h
 * @brief Kernel module (subsystem) registry and dependency-ordered init.
 *
 * A "module" here is a statically linked kernel subsystem, not a loadable
 * binary. Modules declare dependencies by name and are initialized in
 * dependency order by modules_load().
 */
#pragma once
#include "klib/stddef.h"

/** @brief Initialization state of a module. */
typedef enum {
  MODULE_LOADING, /**< init() in progress (used to detect dependency cycles). */
  MODULE_LOADED,  /**< init() finished successfully. */
} module_state_t;

typedef struct module_t module_t;

/** @brief A registered kernel subsystem. */
typedef struct module_t {
  const char *name;                    /**< Unique module name. */
  const char **required_modules_names; /**< NULL-terminated dependency list. */

  int (*init)(module_t *self); /**< Called once, after all dependencies. */
  int (*fini)(void);           /**< Optional teardown hook. */

  void *data_start; /**< Optional payload (e.g. boot module data). */
  size_t data_size; /**< Size of the payload in bytes. */

  module_state_t state;

  module_t *next; /**< Next module in the registry list. */
} module_t;

/**
 * @brief Defines a module and places it in the .modules linker section so
 *        the registry can discover it at boot.
 */
#define ITER_MODULE(name)                                                      \
  static module_t __mod_##name __attribute__((used, section(".modules")))

/**
 * @brief Builds the registry from the .modules section plus an optional
 *        list of dynamically discovered modules.
 */
void modules_init_registry(module_t *dynamic_modules);

/** @brief Initializes every registered module in dependency order. */
void modules_load();
