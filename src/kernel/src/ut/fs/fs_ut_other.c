/*
 * File: fs_ut_other.c
 *
 * Miscellaneous filesystem tests (error handling, persistence, stress tests)
 */

#include "fs/vfs.h"
#include "klib/string.h"
#include "klib/unistd.h"
#include "ksys/fcntl.h"
#include "ksys/stat.h"
#include "ut/ut_framework.h"

#include "ut/fs/fs_ut_helpers.h"
#include "ut/ut_framework.h"

#include "klib/stdio.h"
#include "mm/kmalloc.h"

/*=============================================================================
 * ERROR HANDLING AND EDGE CASE TESTS
 *===========================================================================*/

int ut_error_handling(void) {
  // Try to open non-existent file
  int fd = vfs_open("/nonexistent", O_RDONLY);
  UT_ASSERT_FAIL(fd, "Opening non-existent file should fail");

  // Try to delete non-existent file
  int result = vfs_unlink("/nonexistent");
  UT_ASSERT_FAIL(result, "Deleting non-existent file should fail");

  // Try to remove non-existent directory
  result = vfs_rmdir("/nonexistent");
  UT_ASSERT_FAIL(result, "Removing non-existent directory should fail");

  // Try to create file with invalid path (parent doesn't exist)
  result = vfs_create("/nonexistent/file", 0644);
  UT_ASSERT_FAIL(result, "Creating file in non-existent directory should fail");

  return UT_PASS;
}

int ut_path_boundary(void) {
  // Test root directory operations
  UT_ASSERT_SUCCESS(vfs_create("/rootfile", 0644),
                    "Create file in root directory");

  // Test path with multiple slashes (implementation-dependent)
  UT_ASSERT_SUCCESS(vfs_mkdir("/dir1", 0755), "Create dir1");

  // Some implementations normalize paths, others reject them
  // Just test that it doesn't crash
  int result = vfs_create("/dir1//file", 0644);
  (void)result; // Result may vary by implementation

  return UT_PASS;
}

int ut_permissions(void) {
  // Create file with specific permissions
  UT_ASSERT_SUCCESS(vfs_create("/permtest", 0644),
                    "Create file with permissions");

  // Create directory with specific permissions
  UT_ASSERT_SUCCESS(vfs_mkdir("/permdir", 0755),
                    "Create directory with permissions");

  // Note: Actually checking permission enforcement requires
  // a more complete implementation with user context
  return UT_PASS;
}

/*=============================================================================
 * PERSISTENCE TEST
 *===========================================================================*/

int ut_persistence(void) {
  // Create directory and file with data
  UT_ASSERT_SUCCESS(vfs_mkdir("/dir", 0755), "Create directory");
  UT_ASSERT_SUCCESS(vfs_create("/dir/file1", 0644), "Create file1");

  const char *data = "Persistent data";
  int fd = vfs_open("/dir/file1", O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open file1");

  ssize_t written = vfs_write(fd, data, strlen(data));
  UT_ASSERT_EQUAL(strlen(data), written, "Write data");
  vfs_close(fd);

  // Remount filesystem (calls cleanup and reinit)
  UT_ASSERT_SUCCESS(remount(), "Remount filesystem");

  // Verify file still exists and has correct content
  fd = vfs_open("/dir/file1", O_RDONLY);
  UT_ASSERT_SUCCESS(fd, "File exists after remount");

  char buf[64] = {0};
  ssize_t read_bytes = vfs_read(fd, buf, strlen(data));
  UT_ASSERT_EQUAL(strlen(data), read_bytes, "Read correct amount");
  UT_ASSERT_MEM_EQUAL(data, buf, strlen(data), "Data persisted correctly");

  vfs_close(fd);
  return UT_PASS;
}

/*=============================================================================
 * STRESS TEST
 *===========================================================================*/

int ut_stress_small(void) {
  // Smaller stress test suitable for unit testing
  const int num_files = 10;
  const int file_size = 512;

  char *test_data = kmalloc(file_size);
  UT_ASSERT_NOT_NULL(test_data, "Allocate test data");

  // Fill with pattern
  for (int i = 0; i < file_size; i++) {
    test_data[i] = (char)(i % 256);
  }

  // Create and write files
  for (int i = 0; i < num_files; i++) {
    char filename[64];
    ksnprintf(filename, sizeof(filename), "/stress_%d", i);

    UT_ASSERT_SUCCESS(vfs_create(filename, 0644), "Create stress file");

    int fd = vfs_open(filename, O_RDWR);
    UT_ASSERT_SUCCESS(fd, "Open stress file");

    ssize_t written = vfs_write(fd, test_data, file_size);
    UT_ASSERT_EQUAL(file_size, written, "Write stress file");

    vfs_close(fd);
  }

  // Verify files
  char *read_buffer = kmalloc(file_size);
  UT_ASSERT_NOT_NULL(read_buffer, "Allocate read buffer");

  for (int i = 0; i < num_files; i++) {
    char filename[64];
    ksnprintf(filename, sizeof(filename), "/stress_%d", i);

    int fd = vfs_open(filename, O_RDONLY);
    UT_ASSERT_SUCCESS(fd, "Open stress file for verification");

    ssize_t read_bytes = vfs_read(fd, read_buffer, file_size);
    UT_ASSERT_EQUAL(file_size, read_bytes, "Read stress file");
    UT_ASSERT_MEM_EQUAL(test_data, read_buffer, file_size,
                        "Stress file data integrity");

    vfs_close(fd);
  }

  kfree(test_data);
  kfree(read_buffer);
  return UT_PASS;
}

int ut_concurrent_fd(void) {
  // Test multiple file descriptors open simultaneously
  const char *data1 = "File 1 data";
  const char *data2 = "File 2 data";
  const char *data3 = "File 3 data";

  // Create files
  UT_ASSERT_SUCCESS(vfs_create("/file1", 0644), "Create file1");
  UT_ASSERT_SUCCESS(vfs_create("/file2", 0644), "Create file2");
  UT_ASSERT_SUCCESS(vfs_create("/file3", 0644), "Create file3");

  // Open all three simultaneously
  int fd1 = vfs_open("/file1", O_RDWR);
  int fd2 = vfs_open("/file2", O_RDWR);
  int fd3 = vfs_open("/file3", O_RDWR);

  UT_ASSERT_SUCCESS(fd1, "Open file1");
  UT_ASSERT_SUCCESS(fd2, "Open file2");
  UT_ASSERT_SUCCESS(fd3, "Open file3");

  // Write to all
  UT_ASSERT_EQUAL(strlen(data1), vfs_write(fd1, data1, strlen(data1)),
                  "Write to file1");
  UT_ASSERT_EQUAL(strlen(data2), vfs_write(fd2, data2, strlen(data2)),
                  "Write to file2");
  UT_ASSERT_EQUAL(strlen(data3), vfs_write(fd3, data3, strlen(data3)),
                  "Write to file3");

  // Seek all back to start
  UT_ASSERT_SUCCESS(vfs_seek(fd1, 0, SEEK_SET), "Seek file1");
  UT_ASSERT_SUCCESS(vfs_seek(fd2, 0, SEEK_SET), "Seek file2");
  UT_ASSERT_SUCCESS(vfs_seek(fd3, 0, SEEK_SET), "Seek file3");

  // Read and verify all
  char buf[64];

  UT_ASSERT_EQUAL(strlen(data1), vfs_read(fd1, buf, strlen(data1)),
                  "Read file1");
  UT_ASSERT_MEM_EQUAL(data1, buf, strlen(data1), "File1 data correct");

  UT_ASSERT_EQUAL(strlen(data2), vfs_read(fd2, buf, strlen(data2)),
                  "Read file2");
  UT_ASSERT_MEM_EQUAL(data2, buf, strlen(data2), "File2 data correct");

  UT_ASSERT_EQUAL(strlen(data3), vfs_read(fd3, buf, strlen(data3)),
                  "Read file3");
  UT_ASSERT_MEM_EQUAL(data3, buf, strlen(data3), "File3 data correct");

  // Close all
  UT_ASSERT_SUCCESS(vfs_close(fd1), "Close file1");
  UT_ASSERT_SUCCESS(vfs_close(fd2), "Close file2");
  UT_ASSERT_SUCCESS(vfs_close(fd3), "Close file3");

  return UT_PASS;
}

/*=============================================================================
 * DEFINE THE TEST SUITE
 *===========================================================================*/

static ut_test_case_t tests[] = {
    UT_TEST(ut_error_handling), UT_TEST(ut_path_boundary),
    UT_TEST(ut_permissions),    UT_TEST(ut_persistence),
    UT_TEST(ut_stress_small),   UT_TEST(ut_concurrent_fd)};

// Export the suite
ut_test_suite_t fs_other_suite = {
    .suite_name = "Misc & Edge Cases",
    .setup = fs_test_setup,
    .teardown = fs_test_teardown,
    .suite_setup = fs_suite_setup,
    .suite_teardown = fs_suite_teardown,
    .tests = tests,
    .num_tests = sizeof(tests) / sizeof(tests[0]),
};
