#include "ut/ut_framework.h"

extern ut_test_suite_t ata_suite;

// Add to suites array in main():
int ut_ata_main() {
    ut_test_suite_t suites[] = {ata_suite};

    ut_config_t config = {
        .verbose = 1, .stop_on_fail = 0, .show_passed = 0, .quiet = 0};

    ut_run_suites(suites, 1, &config);
    return 0;
}
