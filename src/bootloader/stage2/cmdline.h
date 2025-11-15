#pragma once
#include <stdbool.h>
#include <stddef.h>

#define MAX_MODULES 16

typedef struct {
  char *path;
} Module;

typedef struct {
  int width;
  int height;
  int bpp;
  int type; // 0 = text, 1 = graphics
  char *mode_name;
} VideoMode;

typedef struct {
  char *root_device;
  char *kernel;
  char *initrd;
  Module modules[MAX_MODULES];
  int module_count;
  bool debug_enabled;
  VideoMode video;
} BCD;

void bcd_cmdline_construct(BCD *bcd, char *cmdline);
void bcd_parse_into(char *boot_config, BCD *out);
void load_initrd(char *initrd_path);
