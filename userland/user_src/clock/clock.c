#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main() {
  // Hide cursor
  printf("\033[?25l");
  fflush(stdout);

  while (1) {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[16];

    if (time(&rawtime) == (time_t)-1) {
      break;
    }
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);

    /* \0337     : DEC Save Cursor (More reliable than \033[s)
       \033[1;72H : Move to Row 1, Col 72
       \033[1;33m : Yellow text
       \033[0m    : Reset colors
       \0338     : DEC Restore Cursor (More reliable than \033[u)
    */
    printf("\0337\033[1;72H\033[1;33m[%s]\033[0m\0338", buffer);

    fflush(stdout);
    sleep(1);
  }

  printf("\033[?25h");
  return 0;
}
