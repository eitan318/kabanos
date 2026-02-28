/*
 * fs_test_helpers.c - Filesystem test helper implementation
 */

#include "ut/fs/fs_ut_helpers.h"
#include "fs/vfs.h"
#include "klib/string.h"
#include "klib/unistd.h"
#include "ksys/fcntl.h"
#include "ksys/stat.h"
#include "ut/ut_framework.h"

/*=============================================================================
 * SETUP/TEARDOWN FUNCTIONS
 *===========================================================================*/

static void rmdir_recursive(const char *path, bool delete_self) {
  int fd = vfs_open(path, O_RDONLY);
  if (fd < 0)
    return;

  VDirEntry entries[32];
  int count;

  while ((count = vfs_iter_dir(fd, entries, 32)) > 0) {
    for (int i = 0; i < count; i++) {
      /* Skip . and .. */
      if (strcmp(entries[i].file_name, ".") == 0)
        continue;
      if (strcmp(entries[i].file_name, "..") == 0)
        continue;

      /* Build full child path */
      char child[PATH_MAX];
      ksnprintf(child, sizeof(child), "%s/%s", path, entries[i].file_name);

      fstat_t st;
      int sfd = vfs_open(child, O_RDONLY);
      if (sfd < 0)
        continue;
      vfs_fstat(sfd, &st);
      vfs_close(sfd);

      if (st.mode == DT_DIR)
        rmdir_recursive(child, true);
      else
        vfs_unlink(child);
    }
  }

  vfs_close(fd);
  if (delete_self)
    vfs_rmdir(path);
}

int fs_test_setup(void) { return 0; }

void fs_test_teardown(void) { rmdir_recursive("/", false); }

int fs_suite_setup(void) { return 0; }

int fs_suite_teardown(void) { return 0; }

/*=============================================================================
 * UTILITY FUNCTIONS
 *===========================================================================*/

int remount() { return 0; }

int fs_create_test_file(const char *path, const char *content) {
  if (vfs_create(path, 0644) < 0) {
    return -1;
  }

  int fd = vfs_open(path, O_RDWR);
  if (fd < 0) {
    return -1;
  }

  ssize_t written = vfs_write(fd, content, strlen(content));
  vfs_close(fd);

  return written == (ssize_t)strlen(content) ? 0 : -1;
}

int fs_verify_file_content(const char *path, const char *expected) {
  int fd = vfs_open(path, O_RDONLY);
  if (fd < 0) {
    return -1;
  }

  char buf[1024];
  ssize_t read_bytes = vfs_read(fd, buf, strlen(expected));
  vfs_close(fd);

  if (read_bytes != (ssize_t)strlen(expected)) {
    return -1;
  }

  return memcmp(buf, expected, strlen(expected)) == 0 ? 0 : -1;
}

int fs_file_exists(const char *path) {
  int fd = vfs_open(path, O_RDONLY);
  if (fd < 0) {
    return 0;
  }
  vfs_close(fd);
  return 1;
}
