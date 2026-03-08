#include <dirent.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  const char *path = ".";

  if (argc > 1)
    path = argv[1];

  printf("ls called, opening dir");

  DIR *dir = opendir(path);
  if (!dir) {
    printf("dir was 0");
    return 1;
  }

  printf("dir opened");

  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    printf("%s\n", entry->d_name);
  }

  printf("doing[3]");

  closedir(dir);
  return 0;
}
