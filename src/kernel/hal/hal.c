#include "hal.h"
#include "arch/i686/gdt.h"
#include "arch/i686/idt.h"
#include "arch/i686/isr/isr.h"
#include "arch/i686/pic.h"
#include "arch/i686/vga_text.h"
#include "drivers/keyboard/keyboard_driver.h"

void hal_init() {
  i686_gdt_init();
  vga_clrscr();
  i686_idt_init();
  i686_isr_init();
  pic_init();

  kbd_init();
}
