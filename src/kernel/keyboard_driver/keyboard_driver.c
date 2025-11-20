#include "keyboard_driver/keyboard_driver.h"
#include "arch/i686/io.h"
#include "arch/i686/isr/isr.h"
#include "include/stdio.h"
#include "utils/queue.h"
#include "arch/i686/pic.h"  

Queue keyboard_queue;

// Scancode -> ASCII mapping (partial, US layout)
static char scancode_to_ascii[128] = {
    0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ', 0};
	
void kbd_init() {
    queue_init(&keyboard_queue);

    // NOTE: pic_init() is called in hal_init(), not here
    
    // Register keyboard interrupt handler
    i686_isr_handler_register(KBD_INT, keyboard_isr_handler);
    
    // Enable keyboard interrupt (IRQ1)
    pic_unmask_irq(KBD_IRQ);
}
	