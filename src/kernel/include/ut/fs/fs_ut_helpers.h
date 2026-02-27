/*
 * fs_test_helpers.h - Filesystem-specific test helpers
 */

#ifndef FS_TEST_HELPERS_H
#define FS_TEST_HELPERS_H

#include "klib/string.h"
#include "klib/unistd.h"
#include "ksys/fcntl.h"
#include "ksys/stat.h"
#include "ut/ut_framework.h"
#include "vfs.h"

/*=============================================================================
 * FILESYSTEM TEST HELPERS - DECLARATIONS
 *===========================================================================*/

// Setup/teardown functions
int fs_test_setup(void);
void fs_test_teardown(void);
int fs_suite_setup(void);
int fs_suite_teardown(void);

// Utility functions
int remount();
int fs_create_test_file(const char *path, const char *content);
int fs_verify_file_content(const char *path, const char *expected);
int fs_file_exists(const char *path);

#endif /* FS_TEST_HELPERS_H */
