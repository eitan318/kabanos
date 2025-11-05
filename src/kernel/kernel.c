#include "include/memory.h"
#include "include/stdio.h"
#include <stdbool.h>
#include <stdint.h>

extern uint8_t __bss_start;
extern uint8_t __end;

void __attribute__((section(".entry"))) start() {
  memset(&__bss_start, 0, (&__end) - (&__bss_start));
  clear_screen();
  printf("hello from 32 bit kernel!!");
  printf("hello from 32 bit kernel!!");
  printf("hello from 32 bit kernel!!");
  printf("hello from 32 bit kernel!!");
  printf("hello from 32 bit kernel!!");
  printf("hello from 32 bit kernel!!");

  for (;;) {
  }
}
