#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TEST_DIR "test_dir"
#define TEST_FILE "test_dir/hello.txt"
#define TEST_DATA "Hello from OS!"

void assert_test(int condition, const char *message) {
  if (condition) {
    printf("[ PASS ] %s\n", message);
  } else {
    printf("[ FAIL ] %s (errno: %d)\n", message, errno);
  }
}

int main() {
  printf("--- Starting VFS Integration Tests ---\n\n");

  // 1. Test getcwd and mkdir
  char initial_cwd[256];
  getcwd(initial_cwd, sizeof(initial_cwd));
  printf("Starting CWD: %s\n", initial_cwd);

  if (mkdir(TEST_DIR, 0755) == 0) {
    assert_test(1, "mkdir test_dir");
  } else {
    assert_test(errno == EEXIST, "mkdir test_dir (allowed if exists)");
  }

  // 2. Test chdir and relative getcwd
  assert_test(chdir(TEST_DIR) == 0, "chdir into test_dir");

  char new_cwd[256];
  getcwd(new_cwd, sizeof(new_cwd));
  assert_test(strstr(new_cwd, TEST_DIR) != NULL,
              "getcwd reflects new directory");

  // 3. Test File Creation and Writing
  int fd = open("hello.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
  assert_test(fd >= 0, "open hello.txt for writing");

  ssize_t written = write(fd, TEST_DATA, strlen(TEST_DATA));
  assert_test(written == strlen(TEST_DATA), "write data to file");
  close(fd);

  // 4. Test Reading and fstat
  fd = open("hello.txt", O_RDONLY);
  struct stat st;
  assert_test(fstat(fd, &st) == 0, "fstat file");
  assert_test(st.st_size == strlen(TEST_DATA), "fstat reports correct size");

  char buf[64];
  memset(buf, 0, 64);
  read(fd, buf, 64);
  assert_test(strcmp(buf, TEST_DATA) == 0, "read data matches written data");
  close(fd);

  // 5. Test Directory Listing (getdents wrapper)
  DIR *dir = opendir(".");
  assert_test(dir != NULL, "opendir current directory");

  struct dirent *entry;
  int found_file = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, "hello.txt") == 0)
      found_file = 1;
    printf("  Found entry: %s\n", entry->d_name);
  }
  assert_test(found_file, "readdir found hello.txt");
  closedir(dir);

  // 6. Test Cleanup (Unlink and Rmdir)
  assert_test(unlink("hello.txt") == 0, "unlink hello.txt");
  assert_test(chdir("..") == 0, "chdir back to parent");
  assert_test(rmdir(TEST_DIR) == 0, "rmdir test_dir");

  printf("\n--- VFS Tests Complete ---\n");
  return 0;
}
