#pragma once
#include "kernel_boot_info.h"
#include "stddef.h"

int module_load(KernelBootInfo *kbi, const char *name);
void *module_get_data(KernelBootInfo *kbi, const char *name,
                      uint32_t *size_out);
