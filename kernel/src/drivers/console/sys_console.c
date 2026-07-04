/**
 * @file sys_console.c
 * @brief Console syscalls.
 */
#include "drivers/console/sys_console.h"
#include "drivers/console/console.h"

long sys_clear(void) {
  con_clear();  
  return 0;
}