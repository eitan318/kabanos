#include <stdio.h>
#include <time.h>
#include <unistd.h>

// --- ANSI Escape Defines ---
#define TERM_SAVE_CURSOR "\0337"
#define TERM_RESTORE_CURSOR "\0338"
#define TERM_HIDE_CURSOR "\033[?25l"
#define TERM_SHOW_CURSOR "\033[?25h"
#define TERM_GOTO_CLOCK_POS "\033[1;71H"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

int main() {
  // Disable cursor to prevent flickering in the corner
  printf(TERM_HIDE_CURSOR);
  fflush(stdout);

  while (1) {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[16];

    if (time(&rawtime) == (time_t)-1) {
      break;
    }

    timeinfo = localtime(&rawtime);
    if (!timeinfo)
      continue;

    // Format: HH:MM:SS
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);

    // Sequence: Save -> Move -> Style -> Print -> Reset -> Restore
    printf(TERM_SAVE_CURSOR TERM_GOTO_CLOCK_POS ANSI_COLOR_GREEN
           "[%s]" ANSI_COLOR_RESET TERM_RESTORE_CURSOR,
           buffer);

    fflush(stdout);
    sleep(1);
  }

  // Restore cursor before exit
  printf(TERM_SHOW_CURSOR);
  fflush(stdout);

  return 0;
}
