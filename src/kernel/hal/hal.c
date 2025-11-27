#include "hal.h"
#include "arch/i686/gdt.h"
#include "arch/i686/idt.h"
#include "arch/i686/isr/isr.h"
#include "arch/i686/vga_text.h"
#include "keyboard_driver/keyboard_driver.h"
#include "arch/i686/pic.h" 

void hal_init() {
    i686_gdt_init();
    vga_clrscr();
    i686_idt_init();
    i686_isr_init();
    
    pic_init();  // Initialize PIC once, before any drivers
    
    kbd_init();
}