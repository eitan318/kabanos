/**
 * @file bcd.h
 * @brief Boot configuration data (boot.cfg) parsing.
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

#define MODULE_PATH_SIZE 256

typedef struct {
  char *kernel;
  char *initrd;
  char **modules_paths;
  int module_count;
  char *cmdline;
} BCD;

void bcd_parse_into(char *boot_config, BCD *out);
