#include <dirent.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  const char *path = ".";

  if (argc > 1)
    path = argv[1];

  DIR *dir = opendir(path);
  if (!dir) {
    return 1;
  }

  printf("dir 1111\n");

  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    printf("%s\n", entry->d_name);
  }

  closedir(dir);
  return 0;
}
