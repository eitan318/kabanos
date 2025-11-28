#pragma once
#include "boot/bootparams.h"
#include "stddef.h"

void modules_init(BootParams *params);
int module_load(const char *name);
void *module_get_data(const char *name, uint32_t *size_out);
