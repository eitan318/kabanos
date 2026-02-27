/*=============================================================================
 * HOW TO ADD YOUR NEW SUITE TO MAIN
 *===========================================================================*/
#include "ut/ut_framework.h"

extern ut_test_suite_t fs_seek_suite;
extern ut_test_suite_t fs_file_basic_suite;
extern ut_test_suite_t fs_symlink_suite;
extern ut_test_suite_t fs_rename_suite;
extern ut_test_suite_t fs_other_suite;

// Add to suites array in main():
int ut_fs_main() {
  ut_test_suite_t suites[] = {
      fs_file_basic_suite, fs_rename_suite, fs_seek_suite,
      fs_symlink_suite,    fs_other_suite,
  };

  ut_config_t config = {
      .verbose = 1, .stop_on_fail = 0, .show_passed = 0, .quiet = 0};

  ut_run_suites(suites, 5, &config);
  return 0;
}
