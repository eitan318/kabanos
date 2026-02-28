#pragma once
#include "klib/stddef.h"
#include "klib/stdint.h"

void initrd_init(void *initrd_start, uint32_t initrd_size);
void *initrd_find_file(const char *filename, uint32_t *size_out);
void initrd_list_files(void);
