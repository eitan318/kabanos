
/*
 * File: fs_ut_rename.c
 *
 * This demonstrates how easy it is to add new test categories
 * using the generic framework.
 */

#include "fs/vfs.h"
#include "klib/string.h"
#include "klib/unistd.h"
#include "ksys/fcntl.h"
#include "ksys/stat.h"
#include "ut/ut_framework.h"

#include "ut/fs/fs_ut_helpers.h"
#include "ut/ut_framework.h"

/*=============================================================================
 * RENAME OPERATION TESTS
 *===========================================================================*/

int ut_rename_same_dir(void) {
  UT_ASSERT_SUCCESS(vfs_create("/oldname", 0644), "Create file");
  int fd = vfs_open("/oldname", O_RDWR);
  const char *data = "Rename test data";
  vfs_write(fd, data, strlen(data));
  vfs_close(fd);

  UT_ASSERT_SUCCESS(vfs_rename("/oldname", "/newname"), "Rename file");

  fd = vfs_open("/oldname", O_RDONLY);
  if (fd != -1) {
    vfs_close(fd);
    return UT_FAIL;
  }

  fd = vfs_open("/newname", O_RDONLY);
  UT_ASSERT_SUCCESS(fd, "Open renamed file");

  char read_buf[64];
  ssize_t n = vfs_read(fd, read_buf, strlen(data));
  if (n != strlen(data) || memcmp(read_buf, data, strlen(data)) != 0) {
    vfs_close(fd);
    return UT_FAIL;
  }

  vfs_close(fd);
  return UT_PASS;
}

int ut_rename_move(void) {
  UT_ASSERT_SUCCESS(vfs_mkdir("/dir1", 0755), "Create dir1");
  UT_ASSERT_SUCCESS(vfs_mkdir("/dir2", 0755), "Create dir2");
  UT_ASSERT_SUCCESS(vfs_create("/dir1/file.txt", 0644), "Create file");

  int fd = vfs_open("/dir1/file.txt", O_RDWR);
  const char *data = "Move test";
  vfs_write(fd, data, strlen(data));
  vfs_close(fd);

  UT_ASSERT_SUCCESS(vfs_rename("/dir1/file.txt", "/dir2/file.txt"),
                    "Move file");

  fd = vfs_open("/dir1/file.txt", O_RDONLY);
  if (fd != -1) {
    vfs_close(fd);
    return UT_FAIL;
  }

  fd = vfs_open("/dir2/file.txt", O_RDONLY);
  UT_ASSERT_SUCCESS(fd, "Open moved file");

  char read_buf[64];
  ssize_t n = vfs_read(fd, read_buf, strlen(data));
  if (n != strlen(data) || memcmp(read_buf, data, strlen(data)) != 0) {
    vfs_close(fd);
    return UT_FAIL;
  }

  vfs_close(fd);
  return UT_PASS;
}

int ut_rename_overwrite(void) {
  UT_ASSERT_SUCCESS(vfs_create("/file1", 0644), "Create file1");
  UT_ASSERT_SUCCESS(vfs_create("/file2", 0644), "Create file2");

  int fd = vfs_open("/file1", O_RDWR);
  const char *data1 = "File 1 data";
  vfs_write(fd, data1, strlen(data1));
  vfs_close(fd);

  fd = vfs_open("/file2", O_RDWR);
  const char *data2 = "File 2 data";
  vfs_write(fd, data2, strlen(data2));
  vfs_close(fd);

  UT_ASSERT_SUCCESS(vfs_rename("/file1", "/file2"), "Rename with overwrite");

  fd = vfs_open("/file1", O_RDONLY);
  if (fd != -1) {
    vfs_close(fd);
    return UT_FAIL;
  }

  fd = vfs_open("/file2", O_RDONLY);
  UT_ASSERT_SUCCESS(fd, "Open destination file");

  char read_buf[64];
  ssize_t n = vfs_read(fd, read_buf, strlen(data1));
  if (n != strlen(data1) || memcmp(read_buf, data1, strlen(data1)) != 0) {
    vfs_close(fd);
    return UT_FAIL;
  }

  vfs_close(fd);
  return UT_PASS;
}

int ut_rename_directory(void) {
  UT_ASSERT_SUCCESS(vfs_mkdir("/olddir", 0755), "Create directory");
  UT_ASSERT_SUCCESS(vfs_create("/olddir/file.txt", 0644), "Create file in dir");

  int fd = vfs_open("/olddir/file.txt", O_RDWR);
  const char *data = "Dir rename test";
  vfs_write(fd, data, strlen(data));
  vfs_close(fd);

  UT_ASSERT_SUCCESS(vfs_rename("/olddir", "/newdir"), "Rename directory");

  fd = vfs_open("/olddir/file.txt", O_RDONLY);
  if (fd != -1) {
    vfs_close(fd);
    return UT_FAIL;
  }

  fd = vfs_open("/newdir/file.txt", O_RDONLY);
  UT_ASSERT_SUCCESS(fd, "Open file in renamed directory");

  char read_buf[64];
  ssize_t n = vfs_read(fd, read_buf, strlen(data));
  if (n != strlen(data) || memcmp(read_buf, data, strlen(data)) != 0) {
    vfs_close(fd);
    return UT_FAIL;
  }

  vfs_close(fd);
  return UT_PASS;
}

/*=============================================================================
 * DEFINE THE TEST SUITE
 *===========================================================================*/

static ut_test_case_t tests[] = {
    UT_TEST(ut_rename_same_dir), UT_TEST(ut_rename_move),
    UT_TEST(ut_rename_overwrite), UT_TEST(ut_rename_directory)};

// Export the suite
ut_test_suite_t fs_rename_suite = {
    .suite_name = "Rename Operations",
    .setup = fs_test_setup,
    .teardown = fs_test_teardown,
    .suite_setup = fs_suite_setup,
    .suite_teardown = fs_suite_teardown,
    .tests = tests,
    .num_tests = sizeof(tests) / sizeof(tests[0]),

};
