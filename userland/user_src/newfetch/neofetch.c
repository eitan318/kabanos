#include <stdio.h>
#include <stdlib.h>

int main() {
  // Define colors using ANSI escape codes
  const char *BLUE = "\033[1;34m";
  const char *RESET = "\033[0m";

  // Your custom ASCII art
  const char *ascii[] = {"           ⢠⣖⣢",
                         "          ⢀⣴⠳⡾⠃",
                         "       ⣴⣍⡩⢱⢉⡉⢆",
                         " ⡠⡐⢦⠩⡝⠉⠖⣒⢄    ⢀⣻⡞⡿⣷⢟⡰⣌⢣⡀",
                         "⢀⠎⣶⣿⠟⠛⢷⡽⣸⠟⠛⢷⡀  ⢀⣴⣿⡷⣏⢷⣹⢾⡻⣎⣶⠇",
                         "⢸⢸⣻⣧⣀⣀⣰⢻⣭⠧⣄⣸⣇  ⠠⣪⣿⣿⣿⣾⣧⣛⣮⢗⠟⠁",
                         "⢸⢰⡿⡼⣭⡟⠈     ⠈⢱⢺ ⢀⡀⡔⣝⣳⢞⡿⣿⣿⣿⣿⣿⠃",
                         "⠈⡌⣿⣳⢧⣏⠴⣀⢆⡡⢌⡌⢟⣘⠮⣼⣹⢾⣽⣻⣾⡽⣯⣿⠛⠁",
                         " ⠘⢾⣽⢿⣿⣾⣷⡾⠶⢟⢪⢳⣭⣻⢾⣽⣿⣿⣿⣿⣿⠗⠁",
                         "   ⢋⡿⡎⠗⡈⡔⡘⣤⢋⡾⢶⣻⣿⣿⣿⣿⣿⠟⠁",
                         " ⢀⠠⠐⢁⠰⣌⢱⢢⡝⣢⢏⡼⣫⣽⣻⣿⠟⠁",
                         " ⡰⠁ ⡰⡎⢷⣈⢷⢇⡾⢱⡎⣷⢇⣷⠿⣾⠁",
                         "⢘⠆⣔⢮⠵⣫⠷⡭⢞⡺⣜⢣⢟⡼⣫⣞⡿⠁",
                         "⣸⢼⡟⣮⢳⣇⠻⣜⢣⡳⡜⣭⠞⣵⣳⠟",
                         "⣬⣶⣿⡛⣈⣩⠘⠳⣎⣧⣳⣝⣶⣛⣾⡍",
                         "⠋⣴⠟⣼⠏⠙⣳⡠⢉⣍⢳⣞⣳⢟⣞⡇",
                         "⢺⠤⢿⡙⢧⢦⠟⣀⣶⣿⡿⣾⢻⣛⢿⠃",
                         "⢧⣛⡟⣿⣶⣶⣾⣿⣿⣷⢿⣱⣏⢮⡓⣧",
                         " ⠳⠟⢡⡿⣞⣳⢯⣛⡞⠙⢿⣳⡞⣧⡛⢶⣷⡀",
                         "  ⢶⣿⣽⣭⣯⢷⡇  ⠈⣿⣿⣷⢿⣻⣯⣷",
                         "  ⢸⣿⣿⣿⣽⡿⡇   ⢹⣿⣿⣿⣻⣾⣽⡇",
                         " ⣰⣿⢿⣿⣿⣿⡟⠁   ⠻⣿⣿⣟⣷⡻⣷",
                         " ⠈⠛⠛⠛⠛⠛⠋      ⠻⠿⠾⠽⠿"};

  // System Info
  const char *user = getenv("USER");
  const char *shell = getenv("SHELL");

  // Print Logic
  printf("\n");
  for (int i = 0; i < 23; i++) {
    printf("%s", ascii[i]);

    // Append info next to specific lines of the ASCII
    if (i == 4)
      printf("   %s%s%s@%sarchlinux%s", BLUE, user, RESET, BLUE, RESET);
    if (i == 5)
      printf("   -----------------------");
    if (i == 6)
      printf("   %sOS:%s      Arch Linux", BLUE, RESET);
    if (i == 7)
      printf("   %sShell:%s   %s", BLUE, RESET, shell);
    if (i == 8)
      printf("   %sWM:%s      Sway/Hyprland", BLUE, RESET);

    printf("\n");
  }
  printf("\n");

  return 0;
}
