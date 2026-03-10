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

  struct dirent *entry;
  printf("starting entries\n");

  while ((entry = readdir(dir)) != NULL) {
    printf("%s\n", entry->d_name);
  }
  printf("finished entries\n");

  closedir(dir);
  return 0;
}
