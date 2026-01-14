#include "ut_framework.h"
#include "include/stdio.h"

/*=============================================================================
 * RUN A SINGLE TEST SUITE (beautiful compact output)
 *===========================================================================*/
int ut_run_suite(ut_test_suite_t *suite, ut_config_t *config) {
  if (!suite || !suite->tests)
    return -1;

  int passed = 0, failed = 0, skipped = 0;

  // Show header if verbose or if there will be output
  if (config->verbose && !config->quiet) {
    debugf("\n%s┌─ %s%s\n", UT_COLOR_BLUE, suite->suite_name, UT_COLOR_RESET);
  }

  if (suite->suite_setup && suite->suite_setup() != 0) {
    if (!config->quiet)
      debugf("%s└─ Suite setup failed%s\n", UT_COLOR_RED, UT_COLOR_RESET);
    return -1;
  }

  // Store failed test names for summary
  const char *failed_tests[suite->num_tests];
  int failed_count = 0;

  // Store all test results to print later if needed
  typedef struct {
    const char *name;
    int result;
  } test_result_t;
  test_result_t test_results[suite->num_tests];

  for (int i = 0; i < suite->num_tests; i++) {
    ut_test_case_t *test = &suite->tests[i];
    test_results[i].name = test->name;

    if (suite->setup && suite->setup() != 0) {
      test_results[i].result = UT_FAIL;
      failed_tests[failed_count++] = test->name;
      failed++;
      if (config->stop_on_fail)
        break;
      continue;
    }

    int result = test->func();
    test_results[i].result = result;

    if (suite->teardown)
      suite->teardown();

    if (result == UT_PASS) {
      passed++;
    } else if (result == UT_SKIP) {
      skipped++;
    } else {
      failed_tests[failed_count++] = test->name;
      failed++;
      if (config->stop_on_fail)
        break;
    }
  }

  // Now print results based on whether there were failures
  if (config->verbose && !config->quiet) {
    int total_run = passed + failed + skipped;

    // If any test failed, print all test results
    if (failed > 0) {
      for (int i = 0; i < total_run; i++) {
        if (test_results[i].result == UT_PASS) {
          debugf("%s│  ✓ %s%s\n", UT_COLOR_GREEN, test_results[i].name,
                 UT_COLOR_RESET);
        } else if (test_results[i].result == UT_SKIP) {
          debugf("%s│  ○ %s (skipped)%s\n", UT_COLOR_YELLOW,
                 test_results[i].name, UT_COLOR_RESET);
        } else {
          debugf("%s│  ✗ %s%s\n", UT_COLOR_RED, test_results[i].name,
                 UT_COLOR_RESET);
        }
      }
    } else if (config->show_passed) {
      // All passed and show_passed is enabled - print them
      for (int i = 0; i < total_run; i++) {
        if (test_results[i].result == UT_PASS) {
          debugf("%s│  ✓ %s%s\n", UT_COLOR_GREEN, test_results[i].name,
                 UT_COLOR_RESET);
        } else if (test_results[i].result == UT_SKIP) {
          debugf("%s│  ○ %s (skipped)%s\n", UT_COLOR_YELLOW,
                 test_results[i].name, UT_COLOR_RESET);
        }
      }
    }
    // If all passed and show_passed is false, don't print individual tests
  } else if (!config->quiet && !config->verbose) {
    // Non-verbose mode: always show failures as they happen
    for (int i = 0; i < passed + failed + skipped; i++) {
      if (test_results[i].result == UT_FAIL) {
        debugf("%s│  ✗ %s%s\n", UT_COLOR_RED, test_results[i].name,
               UT_COLOR_RESET);
      }
    }
  }

  if (suite->suite_teardown && suite->suite_teardown() != 0) {
    if (!config->quiet)
      debugf("%s│  Suite teardown failed%s\n", UT_COLOR_RED, UT_COLOR_RESET);
  }

  // Print summary
  if (!config->quiet) {
    if (config->verbose) {
      // Verbose mode: show full summary with box
      debugf("%s└─ Suite: %s", UT_COLOR_YELLOW, UT_COLOR_RESET);

      if (failed == 0 && skipped == 0) {
        debugf("%s✓ All %d tests passed%s\n", UT_COLOR_GREEN, passed,
               UT_COLOR_RESET);
      } else if (failed == 0) {
        debugf("%s%d passed%s, %s%d skipped%s\n", UT_COLOR_GREEN, passed,
               UT_COLOR_RESET, UT_COLOR_YELLOW, skipped, UT_COLOR_RESET);
      } else {
        debugf("%s%d passed%s, %s%d failed%s", UT_COLOR_GREEN, passed,
               UT_COLOR_RESET, UT_COLOR_RED, failed, UT_COLOR_RESET);
        if (skipped > 0)
          debugf(", %s%d skipped%s", UT_COLOR_YELLOW, skipped, UT_COLOR_RESET);
        debugf("\n");
      }
    } else {
      // Non-verbose mode: only show failures
      if (failed > 0) {
        debugf("%s✗ %s:%s ", UT_COLOR_RED, suite->suite_name, UT_COLOR_RESET);
        for (int i = 0; i < failed_count; i++) {
          debugf("%s", failed_tests[i]);
          if (i < failed_count - 1)
            debugf(", ");
        }
        debugf(" (%d/%d failed)\n", failed, suite->num_tests);
      }
    }
  }

  return failed == 0 ? 0 : -1;
}

/*=============================================================================
 * RUN MULTIPLE TEST SUITES (beautiful compact output)
 *===========================================================================*/
int ut_run_suites(ut_test_suite_t *suites, int num_suites,
                  ut_config_t *config) {
  if (!suites)
    return -1;

  int total_passed = 0, total_failed = 0, total_tests = 0;
  const char *failed_suites[num_suites];
  int failed_suite_count = 0;

  // Show header if verbose
  if (config->verbose && !config->quiet) {
    debugf("%s══════════ Running %d Test Suites ══════════%s\n", UT_COLOR_CYAN,
           num_suites, UT_COLOR_RESET);
  }

  for (int i = 0; i < num_suites; i++) {
    int result = ut_run_suite(&suites[i], config);
    total_tests += suites[i].num_tests;

    if (result == 0) {
      total_passed++;
    } else {
      failed_suites[failed_suite_count++] = suites[i].suite_name;
      total_failed++;
    }

    if (config->stop_on_fail && result != 0)
      break;
  }

  // Final summary
  if (!config->quiet) {
    if (config->verbose) {
      // Verbose mode: full summary with separators
      debugf("%s════════════════════════════════════════%s\n", UT_COLOR_CYAN,
             UT_COLOR_RESET);

      if (total_failed == 0 && total_passed > 0) {
        debugf("%s    ✓ All %d suites passed (%d tests)%s\n", UT_COLOR_GREEN,
               num_suites, total_tests, UT_COLOR_RESET);
      } else {
        debugf("    %s%d/%d suites passed%s",
               total_passed == num_suites ? UT_COLOR_GREEN : UT_COLOR_RESET,
               total_passed, num_suites, UT_COLOR_RESET);

        if (total_failed > 0) {
          debugf(", %s%d failed%s\n", UT_COLOR_RED, total_failed,
                 UT_COLOR_RESET);
          debugf("%s    Failed suites:%s ", UT_COLOR_RED, UT_COLOR_RESET);
          for (int i = 0; i < failed_suite_count; i++) {
            debugf("%s", failed_suites[i]);
            if (i < failed_suite_count - 1)
              debugf(", ");
          }
          debugf("\n");
        } else {
          debugf("\n");
        }
      }

      debugf("%s════════════════════════════════════════%s\n\n", UT_COLOR_CYAN,
             UT_COLOR_RESET);
    } else {
      // Non-verbose mode: one-line summary
      if (total_failed == 0 && total_passed > 0) {
        debugf("%s✓ All tests passed%s (%d suites, %d tests)\n", UT_COLOR_GREEN,
               UT_COLOR_RESET, num_suites, total_tests);
      } else if (total_failed > 0) {
        debugf("%s%d/%d suites passed, %d failed%s\n", UT_COLOR_RED,
               total_passed, num_suites, total_failed, UT_COLOR_RESET);
      }
    }
  }

  return total_failed == 0 ? 0 : -1;
}

/*=============================================================================
 * PRINT TEST SUMMARY (for single suite - now unused, kept for compatibility)
 *===========================================================================*/
void ut_print_summary(int total, int passed, int failed, int skipped) {
  if (failed == 0 && passed > 0) {
    debugf("%s✓ All %d tests passed%s\n", UT_COLOR_GREEN, total,
           UT_COLOR_RESET);
  } else {
    debugf("%s%d passed%s", UT_COLOR_GREEN, passed, UT_COLOR_RESET);
    if (failed > 0)
      debugf(", %s%d failed%s", UT_COLOR_RED, failed, UT_COLOR_RESET);
    if (skipped > 0)
      debugf(", %s%d skipped%s", UT_COLOR_YELLOW, skipped, UT_COLOR_RESET);
    debugf(" (total: %d)\n", total);
  }
}
