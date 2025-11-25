#include "arch/i686/ata.h"
#include "include/stdio.h"
#include "include/string.h"
#include "ut/ut_framework.h"
#include <stdint.h>

int ut_test1(void) {
    uint8_t buff[511];
    for (int i = 0; i < 511; i++) {
        buff[i] = i;
    }
    UT_ASSERT_SUCCESS(ata_write_sector(200, 511, &buff), "Sector writing");
    return UT_PASS;
}

static ut_test_case_t tests[] = {UT_TEST(ut_test1)};

// Export the suite
ut_test_suite_t ata_suite = {
    .suite_name = "ATA sectors",
    .setup = NULL,
    .teardown = NULL,
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .tests = tests,
    .num_tests = sizeof(tests) / sizeof(tests[0]),
};
