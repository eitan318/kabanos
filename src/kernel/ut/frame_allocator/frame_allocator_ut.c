#include "memory_management/pmm.h"
#include "stdio.h"
#include "ut/ut_framework.h"

int ut_basic_allocation(void) {
  uint64_t f1 = pmm_frame_alloc();
  uint64_t f2 = pmm_frame_alloc();

  UT_ASSERT_SUCCESS(f1, "alloc f1");
  UT_ASSERT_SUCCESS(f2, "alloc f2");
  if (f1 == f2 || f1 % FRAME_SIZE || f2 % FRAME_SIZE)
    return UT_FAIL;

  return UT_PASS;
}

int ut_free_and_realloc(void) {
  uint64_t init = frame_get_free_count();
  uint64_t f1 = pmm_frame_alloc();

  UT_ASSERT_SUCCESS(f1, "alloc");
  if (frame_get_free_count() != init - 1)
    return UT_FAIL;

  pmm_frame_free(f1);
  if (frame_get_free_count() != init)
    return UT_FAIL;

  uint64_t f2 = pmm_frame_alloc();
  UT_ASSERT_SUCCESS(f2, "realloc");

  return UT_PASS;
}

int ut_multiple_alloc_free(void) {
#define N 10
  uint64_t frames[N];
  uint64_t init = frame_get_free_count();

  for (int i = 0; i < N; i++) {
    frames[i] = pmm_frame_alloc();
    if (!frames[i])
      return UT_FAIL;
    for (int j = 0; j < i; j++)
      if (frames[i] == frames[j])
        return UT_FAIL;
  }

  if (frame_get_free_count() != init - N)
    return UT_FAIL;

  for (int i = 0; i < N; i++)
    pmm_frame_free(frames[i]);

  if (frame_get_free_count() != init)
    return UT_FAIL;

  return UT_PASS;
}

int ut_double_free(void) {
  uint64_t f = pmm_frame_alloc();
  uint64_t cnt = frame_get_free_count();

  pmm_frame_free(f);
  if (frame_get_free_count() != cnt + 1)
    return UT_FAIL;

  pmm_frame_free(f);
  if (frame_get_free_count() != cnt + 1)
    return UT_FAIL;

  return UT_PASS;
}

int ut_invalid_operations(void) {
  uint64_t init = frame_get_free_count();

  pmm_frame_free(0);
  pmm_frame_free(0x123);
  pmm_frame_free(0xFFFFFFFFFFFF0000UL);

  return (frame_get_free_count() == init) ? UT_PASS : UT_FAIL;
}

int ut_exhaustion(void) {
#define MAX 5000
  static uint64_t frames[MAX];
  uint64_t total = frame_get_free_count();
  uint64_t n = total < MAX ? total : MAX;

  for (uint64_t i = 0; i < n; i++)
    if (!(frames[i] = pmm_frame_alloc()))
      return UT_FAIL;

  if (n == total && pmm_frame_alloc() != 0)
    return UT_FAIL;

  for (uint64_t i = 0; i < n; i++)
    pmm_frame_free(frames[i]);

  return UT_PASS;
}

static int suite_setup() {
  Range mem = {0, 64 * 1024 * 1024};
  Range used[] = {{0, 0x100000}, {0x200000, 0x300000}};
  pmm_init(mem, used, 2);
  return 0;
}

static ut_test_case_t tests[] = {
    UT_TEST(ut_basic_allocation),    UT_TEST(ut_free_and_realloc),
    UT_TEST(ut_multiple_alloc_free), UT_TEST(ut_double_free),
    UT_TEST(ut_invalid_operations),  UT_TEST(ut_exhaustion)};

ut_test_suite_t frame_allocator_suite = {.suite_name = "Frame Allocator",
                                         .suite_setup = suite_setup,
                                         .tests = tests,
                                         .num_tests =
                                             sizeof(tests) / sizeof(tests[0])};
