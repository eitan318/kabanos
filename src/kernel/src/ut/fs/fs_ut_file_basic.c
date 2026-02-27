/*
 * File: fs_ut_file_basic.c
 *
 * Basic file operation tests using the generic framework.
 */

#include "fs/vfs.h"
#include "klib/string.h"
#include "klib/unistd.h"
#include "ksys/fcntl.h"
#include "ksys/stat.h"
#include "ut/ut_framework.h"

#include "klib/stdio.h"

#include "mm/kmalloc.h"
#include "ut/fs/fs_ut_helpers.h"

/*=============================================================================
 * BASIC FILE OPERATION TESTS
 *===========================================================================*/

int ut_create(void) {
  UT_ASSERT_SUCCESS(vfs_create("/testfile", 0644), "File creation");

  // Verify file exists
  int fd = vfs_open("/testfile", O_RDONLY);
  UT_ASSERT_SUCCESS(fd, "File should be openable");
  vfs_close(fd);

  return UT_PASS;
}

int ut_open_close(void) {
  // Create file first
  UT_ASSERT_SUCCESS(vfs_create("/opentest", 0644), "Create file for open test");

  // Open file
  int fd = vfs_open("/opentest", O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open file");

  // Close file
  UT_ASSERT_SUCCESS(vfs_close(fd), "Close file");

  return UT_PASS;
}

int ut_basic_rw(void) {
  // Create and open file
  UT_ASSERT_SUCCESS(vfs_create("/rwtest", 0644), "Create file");

  int fd = vfs_open("/rwtest", O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open file");

  // Write data
  const char *test_data = "Hello, World!";
  ssize_t written = vfs_write(fd, test_data, strlen(test_data));
  UT_ASSERT_EQUAL(strlen(test_data), written, "Write correct amount");

  // Seek to beginning
  UT_ASSERT_SUCCESS(vfs_seek(fd, 0, SEEK_SET), "Seek to beginning");

  // Read data back
  char read_buf[64];
  ssize_t read_bytes = vfs_read(fd, read_buf, strlen(test_data));
  UT_ASSERT_EQUAL(strlen(test_data), read_bytes, "Read correct amount");

  // Verify data
  UT_ASSERT_MEM_EQUAL(test_data, read_buf, strlen(test_data), "Data matches");

  vfs_close(fd);
  return UT_PASS;
}

int ut_append(void) {
  // Create file and write initial data
  UT_ASSERT_SUCCESS(vfs_create("/appendtest", 0644), "Create append test file");

  int fd = vfs_open("/appendtest", O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open append test file");

  const char *initial = "Initial";
  const char *appended = " Appended";

  // Write initial data
  ssize_t written = vfs_write(fd, initial, strlen(initial));
  UT_ASSERT_EQUAL(strlen(initial), written, "Write initial data");

  // Seek to end and append
  vfs_seek(fd, 0, SEEK_END);
  written = vfs_write(fd, appended, strlen(appended));
  UT_ASSERT_EQUAL(strlen(appended), written, "Append data");

  // Read back entire file
  vfs_seek(fd, 0, SEEK_SET);
  char read_buf[64];
  int expected_len = strlen(initial) + strlen(appended);

  ssize_t read_bytes = vfs_read(fd, read_buf, expected_len);
  UT_ASSERT_EQUAL(expected_len, read_bytes, "Read appended file");

  // Verify content
  const char *expected = "Initial Appended";
  UT_ASSERT_MEM_EQUAL(expected, read_buf, expected_len,
                      "Appended content correct");

  vfs_close(fd);
  return UT_PASS;
}

int ut_empty_file(void) {
  // Create empty file
  UT_ASSERT_SUCCESS(vfs_create("/empty", 0644), "Create empty file");

  int fd = vfs_open("/empty", O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open empty file");

  // Try to read from empty file
  char buf[10];
  ssize_t read_bytes = vfs_read(fd, buf, sizeof(buf));
  UT_ASSERT_EQUAL(0, read_bytes, "Reading empty file returns 0");

  // Seek in empty file
  off_t pos = vfs_seek(fd, 0, SEEK_END);
  UT_ASSERT_EQUAL(0, pos, "Seeking to end of empty file returns 0");

  vfs_close(fd);
  return UT_PASS;
}

int ut_large_write(void) {
  UT_ASSERT_SUCCESS(vfs_create("/large", 0644), "Create file");

  int fd = vfs_open("/large", O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open file");

  // Allocate 8KB buffer
  char *buf = kmalloc(8192);
  UT_ASSERT_NOT_NULL(buf, "Allocate buffer");

  // Fill with pattern
  for (int i = 0; i < 8192; i++) {
    buf[i] = (char)(i % 256);
  }

  // Write large data
  ssize_t written = vfs_write(fd, buf, 8192);
  UT_ASSERT_EQUAL(8192, written, "Write 8KB");

  // Verify
  vfs_seek(fd, 0, SEEK_SET);
  char *read_buf = kmalloc(8192);
  UT_ASSERT_NOT_NULL(read_buf, "Allocate read buffer");

  ssize_t read_bytes = vfs_read(fd, read_buf, 8192);
  UT_ASSERT_EQUAL(8192, read_bytes, "Read 8KB back");
  UT_ASSERT_MEM_EQUAL(buf, read_buf, 8192, "Data integrity");

  kfree(buf);
  kfree(read_buf);
  vfs_close(fd);
  return UT_PASS;
}

int ut_unlink(void) {
  const char *data = "test data";

  // Create file
  UT_ASSERT_SUCCESS(vfs_create("/delete_me", 0644), "Create file");

  // Write some data
  int fd = vfs_open("/delete_me", O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open file");
  vfs_write(fd, data, strlen(data));
  vfs_close(fd);

  // Delete file
  UT_ASSERT_SUCCESS(vfs_unlink("/delete_me"), "Delete file");

  // Verify it's gone
  //
  //
  fd = vfs_open("/delete_me", O_RDONLY);
  UT_ASSERT_FAIL(fd, "File should not exist");

  return UT_PASS;
}

int ut_multiple_files(void) {
  const int num_files = 5;
  int fds[num_files];

  // Create and open multiple files
  for (int i = 0; i < num_files; i++) {
    char filename[32];
    ksnprintf(filename, sizeof(filename), "/file%d", i);

    UT_ASSERT_SUCCESS(vfs_create(filename, 0644), "Create file");

    fds[i] = vfs_open(filename, O_RDWR);
    UT_ASSERT_SUCCESS(fds[i], "Open file");

    // Write unique data
    char data[32];
    ksnprintf(data, sizeof(data), "Data for file %d", i);
    ssize_t written = vfs_write(fds[i], data, strlen(data));
    UT_ASSERT_EQUAL(strlen(data), written, "Write to file");
  }

  // Verify and close all files
  for (int i = 0; i < num_files; i++) {
    vfs_seek(fds[i], 0, SEEK_SET);

    char expected[32], actual[32];
    ksnprintf(expected, sizeof(expected), "Data for file %d", i);

    ssize_t read_bytes = vfs_read(fds[i], actual, strlen(expected));
    UT_ASSERT_EQUAL(strlen(expected), read_bytes, "Read from file");
    UT_ASSERT_MEM_EQUAL(expected, actual, strlen(expected), "Data correct");

    UT_ASSERT_SUCCESS(vfs_close(fds[i]), "Close file");
  }

  return UT_PASS;
}

/*=============================================================================
 * DEFINE THE TEST SUITE
 *===========================================================================*/

static ut_test_case_t tests[] = {
    UT_TEST(ut_create), UT_TEST(ut_open_close),    UT_TEST(ut_basic_rw),
    UT_TEST(ut_append), UT_TEST(ut_empty_file),    UT_TEST(ut_large_write),
    UT_TEST(ut_unlink), UT_TEST(ut_multiple_files)};

// Export the suite
ut_test_suite_t fs_file_basic_suite = {
    .suite_name = "File basic Operations",
    .setup = fs_test_setup,
    .teardown = fs_test_teardown,
    .suite_setup = fs_suite_setup,
    .suite_teardown = fs_suite_teardown,
    .tests = tests,
    .num_tests = sizeof(tests) / sizeof(tests[0]),
};
