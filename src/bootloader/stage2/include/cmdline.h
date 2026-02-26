#pragma once
#include "boot/bootparams.h"
#include <stdbool.h>
#include <stddef.h>

#define MODULE_PATH_SIZE 256

typedef struct {
  int width;
  int height;
  int bpp;
  int type; // 0 = text, 1 = graphics
  char *mode_name;
} VideoMode;

typedef struct {
  char *kernel;
  char *initrd;
  char **modules_paths;
  int module_count;
  VideoMode video;
  char *cmdline;
} BCD;

void bcd_cmdline_construct(const char *bcd_cmdline, const int bcd_cmdline_size,
                           char *out);
void bcd_parse_into(char *boot_config, BCD *out);
void load_initrd(char *initrd_path);
