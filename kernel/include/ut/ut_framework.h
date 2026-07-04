/**
 * @file ut_framework.h
 * @brief Generic unit test framework: suites, UT_ASSERT_* macros and
 *        colored reporting. No external dependencies.
 *
 * Test functions return UT_PASS / UT_FAIL / UT_SKIP; the UT_ASSERT_*
 * macros return UT_FAIL from the calling test on violation.
 */

#pragma once

#include "klib/stdarg.h"
#include "klib/stdio.h"

#define UT_PASS 0
#define UT_FAIL -1
#define UT_SKIP -2

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

typedef int (*ut_test_func_t)(void);

/** @brief A single named test. */
typedef struct {
  const char *name;
  ut_test_func_t func;
} ut_test_case_t;

/** @brief A group of tests with optional fixtures. */
typedef struct {
  const char *suite_name;
  ut_test_case_t *tests;
  int num_tests;
  int (*setup)(void);          /**< Called before each test (optional). */
  void (*teardown)(void);      /**< Called after each test (optional). */
  int (*suite_setup)(void);    /**< Called once before the suite (optional). */
  int (*suite_teardown)(void); /**< Called once after the suite (optional). */
} ut_test_suite_t;

/** @brief Runner output/behavior options. */
typedef struct {
  int verbose;      /**< Print detailed output. */
  int stop_on_fail; /**< Stop on first failure. */
  int show_passed;  /**< Show passed test names. */
  int quiet;        /**< Minimal output. */
} ut_config_t;

#define UT_ASSERT(condition, msg)                                              \
  do {                                                                         \
    if (!(condition)) {                                                        \
      kdebugf("%sFAIL%s: %s (line %d)\n", UT_COLOR_RED, UT_COLOR_RESET, (msg), \
              __LINE__);                                                       \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_EQUAL(expected, actual, msg)                                 \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      kdebugf("%sFAIL%s: %s - Expected: %d, Got: %d (line %d)\n",              \
              UT_COLOR_RED, UT_COLOR_RESET, (msg), (expected), (actual),       \
              __LINE__);                                                       \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_NOT_EQUAL(val1, val2, msg)                                   \
  do {                                                                         \
    if ((val1) == (val2)) {                                                    \
      kdebugf("%sFAIL%s: %s - Values should not be equal: %d (line %d)\n",     \
              UT_COLOR_RED, UT_COLOR_RESET, (msg), (val1), __LINE__);          \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_STR_EQUAL(expected, actual, msg)                             \
  do {                                                                         \
    if (strcmp((expected), (actual)) != 0) {                                   \
      kdebugf("%sFAIL%s: %s - Expected: '%s', Got: '%s' (line %d)\n",          \
              UT_COLOR_RED, UT_COLOR_RESET, (msg), (expected), (actual),       \
              __LINE__);                                                       \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_NOT_NULL(ptr, msg)                                           \
  do {                                                                         \
    if ((ptr) == NULL) {                                                       \
      kdebugf("%sFAIL%s: %s - Pointer is NULL (line %d)\n", UT_COLOR_RED,      \
              UT_COLOR_RESET, (msg), __LINE__);                                \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_NULL(ptr, msg)                                               \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      kdebugf("%sFAIL%s: %s - Pointer should be NULL (line %d)\n",             \
              UT_COLOR_RED, UT_COLOR_RESET, (msg), __LINE__);                  \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_SUCCESS(result, msg)                                         \
  do {                                                                         \
    if ((result) < 0) {                                                        \
      kdebugf("%sFAIL%s: %s - Operation failed with code %d (line %d)\n",      \
              UT_COLOR_RED, UT_COLOR_RESET, (msg), (result), __LINE__);        \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_FAIL(result, msg)                                            \
  do {                                                                         \
    if ((result) >= 0) {                                                       \
      kdebugf("%sFAIL%s: %s - Operation should have failed (line %d)\n",       \
              UT_COLOR_RED, UT_COLOR_RESET, (msg), __LINE__);                  \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_SUCCESS_CLEAN(result, msg, cleanup)                          \
  do {                                                                         \
    if ((result) < 0) {                                                        \
      kdebugf("%sFAIL%s: %s - code %d (line %d)\n", UT_COLOR_RED,              \
              UT_COLOR_RESET, (msg), (result), __LINE__);                      \
      cleanup;                                                                 \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

#define UT_ASSERT_MEM_EQUAL(expected, actual, size, msg)                       \
  do {                                                                         \
    if (memcmp((expected), (actual), (size)) != 0) {                           \
      kdebugf("%sFAIL%s: %s - Memory contents differ (line %d)\n",             \
              UT_COLOR_RED, UT_COLOR_RESET, (msg), __LINE__);                  \
      return UT_FAIL;                                                          \
    }                                                                          \
  } while (0)

/** @brief Skips a test (useful for temporarily disabling tests). */
#define UT_SKIP_TEST(msg)                                                      \
  do {                                                                         \
    kdebugf("%sSKIP%s: %s\n", UT_COLOR_YELLOW, UT_COLOR_RESET, (msg));         \
    return UT_SKIP;                                                            \
  } while (0)

/** @brief Builds a ut_test_case_t entry from a function name. */
#define UT_TEST(func)                                                          \
  { #func, func }

/** @brief Runs one suite; returns the number of failed tests. */
int ut_run_suite(ut_test_suite_t *suite, ut_config_t *config);

/** @brief Runs multiple suites and prints a combined summary. */
int ut_run_suites(ut_test_suite_t *suites, int num_suites, ut_config_t *config);

void ut_print_summary(int total, int passed, int failed, int skipped);

static inline ut_config_t ut_default_config(void) {
  ut_config_t config = {
      .verbose = 1, .stop_on_fail = 0, .show_passed = 0, .quiet = 0};
  return config;
}
