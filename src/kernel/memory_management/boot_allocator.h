#pragma once
#include "utils/math.h"
#include "utils/range.h"
#include <stddef.h>

void *boot_alloc(size_t size);
Range boot_alloc_get_used_range();
