/*
 * ut_framework.h - Generic Unit Test Framework
 *
 * A simple, reusable unit testing framework for C projects.
 * No external dependencies required.
 */

#ifndef UT_FRAMEWORK_H
#define UT_FRAMEWORK_H

#include "include/stdio.h"
#include <stdarg.h>
#include <stdlib.h>

/*=============================================================================
 * TEST RESULT CODES
 *===========================================================================*/
#define UT_PASS 0
#define UT_FAIL -1
#define UT_SKIP -2

/*=============================================================================
 * COLOR OUTPUT (can be disabled by defining UT_NO_COLOR)
 *===========================================================================*/
#ifndef UT_NO_COLOR
#define UT_COLOR_RED "\x1b[31m"
#define UT_COLOR_GREEN "\x1b[32m"
#define UT_COLOR_YELLOW "\x1b[33m"
#define UT_COLOR_BLUE "\x1b[34m"
#define UT_COLOR_CYAN "\x1b[36m"
#define UT_COLOR_RESET "\x1b[0m"
#else
#define UT_COLOR_RED ""
#define UT_COLOR_GREEN ""
#define UT_COLOR_YELLOW ""
#define UT_COLOR_BLUE ""
#define UT_COLOR_CYAN ""
#define UT_COLOR_RESET ""
#endif

/*=============================================================================
 * TEST FUNCTION SIGNATURE
 *===========================================================================*/
typedef int (*ut_test_func_t)(void);

/*=============================================================================
 * TEST CASE STRUCTURE
 *===========================================================================*/
typedef struct {
  const char *name;
  ut_test_func_t func;
} ut_test_case_t;

/*=============================================================================
 * TEST SUITE STRUCTURE
 *===========================================================================*/
typedef struct {
  const char *suite_name;
  ut_test_case_t *tests;
  int num_tests;
  int (*setup)(void);          // Called before each test (optional)
  void (*teardown)(void);      // Called after each test (optional)
  int (*suite_setup)(void);    // Called once before suite (optional)
  int (*suite_teardown)(void); // Called once after suite (optional)
} ut_test_suite_t;

/*=============================================================================
 * TEST CONFIGURATION
 *===========================================================================*/
typedef struct {
  int verbose;      // Print detailed output
  int stop_on_fail; // Stop on first failure
  int show_passed;  // Show passed test names
  int quiet;        // Minimal output
} ut_config_t;

/*=============================================================================
 * ASSERTION MACROS
 *===========================================================================*/

#define UT_ASSERT(condition, msg)                                              \
  do {                                                                         \
    if (!(condition)) {                                                        \
      debugf("%sFAIL%s: %s (line %d)\n", UT_COLOR_RED, UT_COLOR_RESET, (msg),  \
             __LINE__);                                                        \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_EQUAL(expected, actual, msg)                                 \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      debugf("%sFAIL%s: %s - Expected: %d, Got: %d (line %d)\n", UT_COLOR_RED, \
             UT_COLOR_RESET, (msg), (expected), (actual), __LINE__);           \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_NOT_EQUAL(val1, val2, msg)                                   \
  do {                                                                         \
    if ((val1) == (val2)) {                                                    \
      debugf("%sFAIL%s: %s - Values should not be equal: %d (line %d)\n",      \
             UT_COLOR_RED, UT_COLOR_RESET, (msg), (val1), __LINE__);           \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_STR_EQUAL(expected, actual, msg)                             \
  do {                                                                         \
    if (strcmp((expected), (actual)) != 0) {                                   \
      debugf("%sFAIL%s: %s - Expected: '%s', Got: '%s' (line %d)\n",           \
             UT_COLOR_RED, UT_COLOR_RESET, (msg), (expected), (actual),        \
             __LINE__);                                                        \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_NOT_NULL(ptr, msg)                                           \
  do {                                                                         \
    if ((ptr) == NULL) {                                                       \
      debugf("%sFAIL%s: %s - Pointer is NULL (line %d)\n", UT_COLOR_RED,       \
             UT_COLOR_RESET, (msg), __LINE__);                                 \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_NULL(ptr, msg)                                               \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      debugf("%sFAIL%s: %s - Pointer should be NULL (line %d)\n",              \
             UT_COLOR_RED, UT_COLOR_RESET, (msg), __LINE__);                   \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_SUCCESS(result, msg)                                         \
  do {                                                                         \
    if ((result) < 0) {                                                        \
      debugf("%sFAIL%s: %s - Operation failed with code %d (line %d)\n",       \
             UT_COLOR_RED, UT_COLOR_RESET, (msg), (result), __LINE__);         \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_FAIL(result, msg)                                            \
  do {                                                                         \
    if ((result) >= 0) {                                                       \
      debugf("%sFAIL%s: %s - Operation should have failed (line %d)\n",        \
             UT_COLOR_RED, UT_COLOR_RESET, (msg), __LINE__);                   \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_SUCCESS_CLEAN(result, msg, cleanup)                          \
  do {                                                                         \
    if ((result) < 0) {                                                        \
      debugf("%sFAIL%s: %s - code %d (line %d)\n", UT_COLOR_RED,               \
             UT_COLOR_RESET, (msg), (result), __LINE__);                       \
      cleanup;                                                                 \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_MEM_EQUAL(expected, actual, size, msg)                       \
  do {                                                                         \
    if (memcmp((expected), (actual), (size)) != 0) {                           \
      debugf("%sFAIL%s: %s - Memory contents differ (line %d)\n",              \
             UT_COLOR_RED, UT_COLOR_RESET, (msg), __LINE__);                   \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

/*=============================================================================
 * HELPER MACROS
 *===========================================================================*/

// Skip a test (useful for temporarily disabling tests)
#define UT_SKIP_TEST(msg)                                                      \
  do {                                                                         \
    debugf("%sSKIP%s: %s\n", UT_COLOR_YELLOW, UT_COLOR_RESET, (msg));          \
    return UT_SKIP;                                                            \
  } while (0)

// Define a test array
#define UT_TEST(func)                                                          \
  { #func, func }

/*=============================================================================
 * TEST RUNNER FUNCTIONS
 *===========================================================================*/

// Run a single test suite
int ut_run_suite(ut_test_suite_t *suite, ut_config_t *config);

// Run multiple test suites
int ut_run_suites(ut_test_suite_t *suites, int num_suites, ut_config_t *config);

// Print test summary
void ut_print_summary(int total, int passed, int failed, int skipped);

/*=============================================================================
 * DEFAULT CONFIGURATION
 *===========================================================================*/
static inline ut_config_t ut_default_config(void) {
  ut_config_t config = {
      .verbose = 1, .stop_on_fail = 0, .show_passed = 0, .quiet = 0};
  return config;
}

#endif /* UT_FRAMEWORK_H */
