#pragma once

#include "ut/ut_framework.h"

// Export the test suite
extern ut_test_suite_t pcb_suite;

// Main function to run PCB tests
static inline int ut_pcb_main(void) {
  ut_test_suite_t suites[] = {pcb_suite};

  ut_config_t config = {
      .verbose = 1, .stop_on_fail = 0, .show_passed = 0, .quiet = 0};

  int result = ut_run_suites(suites, 1, &config);

  debugf("\n");
  return result;
}
