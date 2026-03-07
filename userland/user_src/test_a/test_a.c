#include <stddef.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

void main() {
  int status;
  printf("Test 1: Blocking Wait\n");
  pid_t child = fork();

  if (child == 0) {
    printf("  Child: Sleeping 2 seconds...\n");
    sleep(2);
    printf("  Child: Exiting with code 42\n");
    _exit(42);
  } else {
    printf("  Parent: Waiting for child %d...\n", child);
    pid_t waited_pid = waitpid(child, &status, 0);
    int exit_val = (status >> 8) & 0xFF;
    printf("  Parent: Collected child %d. Exit status: %d\n", waited_pid,
           exit_val);
  }

  printf("\nTest 2: WNOHANG (Non-blocking)\n");
  child = fork();
  if (child == 0) {
    sleep(5);
    _exit(0);
  } else {
    // This should return 0 immediately because child is still sleeping
    pid_t result = waitpid(child, &status, 1); // 1 is WNOHANG
    if (result == 0) {
      printf("  Success: WNOHANG returned 0 (child not ready)\n");
    } else {
      printf("  Fail: WNOHANG returned %d instead of 0\n", result);
    }
  }

  printf("\nTest 3: Invalid PID (ECHILD)\n");
  pid_t bad_pid = waitpid(9999, &status, 0);
  if (bad_pid < 0) {
    printf("  Success: waitpid correctly failed for non-existent child\n");
  }
}
