#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

// int proc_spawn(char *name) {
//   pid_t pid = fork();
//
//   if (pid < 0) {
//     perror("fork failed");
//     return 1;
//   }
//
//   if (pid == 0) {
//     execve(name, NULL, NULL);
//     perror("execve failed");
//     _exit(1);
//   }
//   return 0;
// }
//
int main(void) {
  printf("hello");
  // proc_spawn("test_b.elf");
  printf("hello");

  return 0;
}
