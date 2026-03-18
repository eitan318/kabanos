#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Recursive function to delete files and directories
int remove_recursive(const char *path) {
  struct stat path_stat;

  // Get stats to see if it's a file or directory
  if (lstat(path, &path_stat) < 0)
    return -1;

  // If it's not a directory, just unlink it (delete file)
  if (!S_ISDIR(path_stat.st_mode)) {
    return unlink(path);
  }

  // It's a directory: Open it and iterate through contents
  DIR *d = opendir(path);
  if (!d)
    return -1;

  struct dirent *p;
  int r = 0;

  while (!r && (p = readdir(d))) {
    // Skip the special "." and ".." directories to avoid infinite loops
    if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
      continue;
    }

    // Build the full path for the child item
    char *buf = malloc(strlen(path) + strlen(p->d_name) + 2);
    if (buf) {
      sprintf(buf, "%s/%s", path, p->d_name);
      r = remove_recursive(buf); // RECURSION
      free(buf);
    }
  }

  closedir(d);

  // After children are deleted, delete the now-empty directory
  if (!r) {
    r = rmdir(path);
  }

  return r;
}

int main(int argc, char *argv[]) {
  int recursive = 0;
  char *target_path = NULL;

  if (argc == 3 && strcmp(argv[1], "-r") == 0) {
    recursive = 1;
    target_path = argv[2];
  } else if (argc == 2) {
    target_path = argv[1];
  } else {
    fprintf(stderr, "Usage: %s [-r] <path>\n", argv[0]);
    return 1;
  }

  int result;
  if (recursive) {
    result = remove_recursive(target_path);
  } else {
    result = rmdir(target_path);
  }

  if (result != 0) {
    fprintf(stderr, "rmdir: failed to remove '%s': %s\n", target_path,
            strerror(errno));
    return 1;
  }

  return 0;
}
