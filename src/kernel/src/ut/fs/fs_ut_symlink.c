
/*
 * File: fs_ut_symlink.c
 *
 * This demonstrates how easy it is to add new test categories
 * using the generic framework.
 */

#include "klib/string.h"
#include "ut/fs/fs_ut_helpers.h"
#include "ut/ut_framework.h"

/*=============================================================================
 * SYMLINK OPERATION TESTS
 *===========================================================================*/

int ut_symlink_basic(void) {
  UT_ASSERT_SUCCESS(vfs_create("/target_file", 0644), "Target file creation");

  int fd = vfs_open("/target_file", O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open target file");
  const char *data = "Target content";
  UT_ASSERT_SUCCESS(vfs_write(fd, data, strlen(data)), "Write to target");
  vfs_close(fd);

  // Create symlink
  UT_ASSERT_SUCCESS(vfs_symlink("/target_file", "/link_to_target"),
                    "Symlink creation");

  // Read symlink target
  char buf[256];
  ssize_t len = vfs_readlink("/link_to_target", buf, sizeof(buf));
  if (len < 0)
    return UT_FAIL;
  buf[len] = '\0';
  if (strcmp(buf, "/target_file") != 0)
    return UT_FAIL;

  // Open through symlink
  fd = vfs_open("/link_to_target", O_RDONLY);
  UT_ASSERT_SUCCESS(fd, "Open via symlink");

  char read_buf[64];
  ssize_t n = vfs_read(fd, read_buf, strlen(data));
  if (n != strlen(data) || memcmp(read_buf, data, strlen(data)) != 0) {
    vfs_close(fd);
    return UT_FAIL;
  }

  vfs_close(fd);
  return UT_PASS;
}

int ut_symlink_relative(void) {
  UT_ASSERT_SUCCESS(vfs_mkdir("/dir1", 0755), "Create dir1");
  UT_ASSERT_SUCCESS(vfs_mkdir("/dir2", 0755), "Create dir2");
  UT_ASSERT_SUCCESS(vfs_create("/dir1/file.txt", 0644), "Create file");

  int fd = vfs_open("/dir1/file.txt", O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open file");
  const char *data = "Relative symlink test";
  UT_ASSERT_SUCCESS(vfs_write(fd, data, strlen(data)), "Write file");
  vfs_close(fd);

  UT_ASSERT_SUCCESS(vfs_symlink("../dir1/file.txt", "/dir2/link_to_file"),
                    "Create relative symlink");

  fd = vfs_open("/dir2/link_to_file", O_RDONLY);
  UT_ASSERT_SUCCESS(fd, "Open relative symlink");

  char read_buf[64];
  ssize_t n = vfs_read(fd, read_buf, strlen(data));
  if (n != strlen(data) || memcmp(read_buf, data, strlen(data)) != 0) {
    vfs_close(fd);
    return UT_FAIL;
  }

  vfs_close(fd);
  return UT_PASS;
}

int ut_symlink_chain(void) {

  UT_ASSERT_SUCCESS(vfs_create("/real_file", 0644), "Create target file");
  int fd = vfs_open("/real_file", O_RDWR);
  const char *data = "Chain end data";
  UT_ASSERT_SUCCESS(vfs_write(fd, data, strlen(data)), "Write data");
  vfs_close(fd);

  UT_ASSERT_SUCCESS(vfs_symlink("/real_file", "/link3"), "link3");
  UT_ASSERT_SUCCESS(vfs_symlink("/link3", "/link2"), "link2");
  UT_ASSERT_SUCCESS(vfs_symlink("/link2", "/link1"), "link1");

  fd = vfs_open("/link1", O_RDONLY);
  UT_ASSERT_SUCCESS(fd, "Open symlink chain");

  char read_buf[64];
  ssize_t n = vfs_read(fd, read_buf, strlen(data));
  if (n != strlen(data) || memcmp(read_buf, data, strlen(data)) != 0) {
    vfs_close(fd);
    return UT_FAIL;
  }

  vfs_close(fd);
  return UT_PASS;
}

int ut_symlink_to_dir(void) {
  UT_ASSERT_SUCCESS(vfs_mkdir("/mydir", 0755), "Create directory");
  UT_ASSERT_SUCCESS(vfs_create("/mydir/file.txt", 0644), "Create file");

  UT_ASSERT_SUCCESS(vfs_symlink("/mydir", "/link_to_dir"),
                    "Symlink to directory");

  int fd = vfs_open("/link_to_dir/file.txt", O_RDONLY);
  UT_ASSERT_SUCCESS(fd, "Open file through directory symlink");
  vfs_close(fd);

  return UT_PASS;
}

int ut_symlink_broken(void) {
  UT_ASSERT_SUCCESS(vfs_symlink("/nonexistent", "/broken_link"),
                    "Create broken symlink");

  char buf[256];
  ssize_t len = vfs_readlink("/broken_link", buf, sizeof(buf));
  if (len < 0)
    return UT_FAIL;
  buf[len] = '\0';
  if (strcmp(buf, "/nonexistent") != 0)
    return UT_FAIL;

  int fd = vfs_open("/broken_link", O_RDONLY);
  if (fd != -1) {
    vfs_close(fd);
    return UT_FAIL;
  }

  return UT_PASS;
}

/*=============================================================================
 * DEFINE THE TEST SUITE
 *===========================================================================*/

static ut_test_case_t tests[] = {
    UT_TEST(ut_symlink_basic), UT_TEST(ut_symlink_relative),
    UT_TEST(ut_symlink_chain), UT_TEST(ut_symlink_to_dir),
    UT_TEST(ut_symlink_broken)};

// Export the suite
ut_test_suite_t fs_symlink_suite = {
    .suite_name = "Symlink Operations",
    .setup = fs_test_setup,
    .teardown = fs_test_teardown,
    .suite_setup = fs_suite_setup,
    .suite_teardown = fs_suite_teardown,
    .tests = tests,
    .num_tests = sizeof(tests) / sizeof(tests[0]),
};
