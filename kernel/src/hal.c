#include "hal.h"
#include "modules.h"

static const char *hal_deps[] = {NULL};

ITER_MODULE(hal) = {
    .name = "hal",
    .required_modules_names = hal_deps,
    .init = &hal_arch_init,
};
