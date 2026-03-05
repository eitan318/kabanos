#include <fcntl.h>
#include <stddef.h>

extern int main(int argc, char **argv, char **envp);
extern void _exit(int code);

void _start() {
  int ex = main(0, NULL, NULL);
  _exit(ex);
}
