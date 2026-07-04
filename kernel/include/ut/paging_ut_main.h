/**
 * @file paging_ut_main.h
 * @brief Entry point running the paging test suite.
 */
#pragma once
#include "ut/ut_framework.h"

extern ut_test_suite_t paging_suite;

int ut_paging_main() {
  ut_test_suite_t suites[] = {paging_suite};
  ut_config_t config = {
      .verbose = 1, .stop_on_fail = 0, .show_passed = 0, .quiet = 0};
  ut_run_suites(suites, 1, &config);
  return 0;
}
