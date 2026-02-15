#include "hal.h"
#include "modules/modules.h"

static const char *hal_deps[] = {NULL};

ITER_MODULE(hal) = {
    .name = "hal",
    .required = hal_deps,
    .init = &hal_arch_init,
};
