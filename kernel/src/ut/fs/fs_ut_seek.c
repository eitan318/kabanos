/*
 * Example: How to create a new test file
 * File: fs_seek_tests.c
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
 * SEEK OPERATION TESTS
 *===========================================================================*/

int test_seek_set(void) {
  const char *data = "0123456789ABCDEF";

  UT_ASSERT_SUCCESS(fs_create_test_file("/seektest", data), "Create test file");

  int fd = vfs_open("/seektest", NULL, O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open file");

  // Seek to position 5
  off_t pos = vfs_seek(fd, 5, SEEK_SET);
  UT_ASSERT_EQUAL(5, pos, "SEEK_SET to position 5");

  // Read one character
  char c;
  ssize_t n = vfs_read(fd, &c, 1);
  UT_ASSERT_EQUAL(1, n, "Read one character");
  UT_ASSERT_EQUAL('5', c, "Character at position 5");

  vfs_close(fd);
  return UT_PASS;
}

int test_seek_cur(void) {
  const char *data = "0123456789ABCDEF";

  UT_ASSERT_SUCCESS(fs_create_test_file("/seektest", data), "Create test file");

  int fd = vfs_open("/seektest", NULL, O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open file");

  // Read 3 bytes (now at position 3)
  char buf[4];
  vfs_read(fd, buf, 3);

  // Seek forward 2 from current position
  off_t pos = vfs_seek(fd, 2, SEEK_CUR);
  UT_ASSERT_EQUAL(5, pos, "SEEK_CUR forward 2");

  // Read one character
  char c;
  vfs_read(fd, &c, 1);
  UT_ASSERT_EQUAL('5', c, "Character after SEEK_CUR");

  vfs_close(fd);
  return UT_PASS;
}

int test_seek_end(void) {
  const char *data = "0123456789ABCDEF";
  int len = strlen(data);

  UT_ASSERT_SUCCESS(fs_create_test_file("/seektest", data), "Create test file");

  int fd = vfs_open("/seektest", NULL, O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open file");

  // Seek to 2 bytes before end
  off_t pos = vfs_seek(fd, -2, SEEK_END);
  UT_ASSERT_EQUAL(len - 2, pos, "SEEK_END -2");

  // Read one character
  char c;
  vfs_read(fd, &c, 1);
  UT_ASSERT_EQUAL('E', c, "Second to last character");

  vfs_close(fd);
  return UT_PASS;
}

int test_seek_beyond_eof(void) {
  const char *data = "short";

  UT_ASSERT_SUCCESS(fs_create_test_file("/seektest", data), "Create test file");

  int fd = vfs_open("/seektest", NULL, O_RDWR);
  UT_ASSERT_SUCCESS(fd, "Open file");

  // Try to seek beyond EOF (behavior depends on implementation)
  off_t pos = vfs_seek(fd, 100, SEEK_SET);

  // Most implementations allow this but reads return 0
  if (pos >= 0) {
    char buf[10];
    ssize_t n = vfs_read(fd, buf, 10);
    UT_ASSERT_EQUAL(0, n, "Read beyond EOF returns 0");
  }

  vfs_close(fd);
  return UT_PASS;
}

/*=============================================================================
 * DEFINE THE TEST SUITE
 *===========================================================================*/
static ut_test_case_t tests[] = {UT_TEST(test_seek_set), UT_TEST(test_seek_cur),
                                 UT_TEST(test_seek_end),
                                 UT_TEST(test_seek_beyond_eof)};

// Export the suite
ut_test_suite_t fs_seek_suite = {
    .suite_name = "File seek Operations",
    .setup = fs_test_setup,
    .teardown = fs_test_teardown,
    .suite_setup = fs_suite_setup,
    .suite_teardown = fs_suite_teardown,
    .tests = tests,
    .num_tests = sizeof(tests) / sizeof(tests[0]),

};

/*=============================================================================
 * MAKEFILE EXAMPLE
 *===========================================================================*/

/*
# Makefile for filesystem tests

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I../include
LDFLAGS = -L../lib -lvfs -lmyfs -ldisk

# Test framework
FRAMEWORK_OBJS = ut_framework.o

# Test helper objects
HELPER_OBJS = fs_test_helpers.o

# Individual test suite objects
TEST_OBJS = fs_basic_tests.o \
            fs_symlink_tests.o \
            fs_rename_tests.o \
            fs_iter_tests.o \
            fs_seek_tests.o \
            fs_large_file_tests.o

# Main test runner
MAIN_OBJ = fs_test_main.o

# Target executable
TARGET = fs_tests

all: $(TARGET)

$(TARGET): $(FRAMEWORK_OBJS) $(HELPER_OBJS) $(TEST_OBJS) $(MAIN_OBJ)
        $(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
        $(CC) $(CFLAGS) -c $<

clean:
        rm -f *.o $(TARGET)

run: $(TARGET)
        ./$(TARGET)

run-verbose: $(TARGET)
        ./$(TARGET) -v

.PHONY: all clean run run-verbose
*/
