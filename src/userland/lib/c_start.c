#include "stdio/stdio_internal.h"

extern int main(int argc, char **argv, char **envp);

int c_start() {
  stdio_init();
  return main(0, NULL, NULL);
}
