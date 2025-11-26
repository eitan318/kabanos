#include "keyboard_driver/keyboard_driver.h"
#include "arch/i686/io.h"
#include "arch/i686/isr/isr.h"
#include "include/stdio.h"
#include "utils/queue.h"
#include "arch/i686/pic.h"  

Queue keyboard_queue;

// Modifier key states
static int shift_pressed = 0;
static int ctrl_pressed = 0;

// Partial US layout scancode -> ASCII
static char scancode_to_ascii[128] = {
    0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ', 0};

// Shifted characters (for US keyboard)
static char scancode_to_ascii_shift[128] = {
    0,   27,  '!',  '@',  '#', '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{',  '}',  '\n', 0,  'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L',  ':',  '"', '~', 0,  '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M',  '<',  '>', '?', 0,  '*',  0,   ' ', 0};

#define MAX_PRESS_SCANCODE 0x80
#define KEYBOARD_PORT 0x60

static void keyboard_isr_handler(Registers* regs) {
    uint8_t scancode = i686_inb(KEYBOARD_PORT);

    // Handle key release (scancode >= 0x80)
    int key_released = scancode & 0x80;
    uint8_t keycode = scancode & 0x7F;

    // Update modifier states
    if (keycode == KBD_SHIFT || keycode == KBD_SHIFT_R) {
        shift_pressed = !key_released;
        goto eoi;  // do not enqueue shift itself
    }
    if (keycode == KBD_CTRL) {
        ctrl_pressed = !key_released;
        goto eoi;  // do not enqueue ctrl itself
    }

    // Only handle key press events
    if (!key_released) {
        char key_ascii = shift_pressed ? scancode_to_ascii_shift[keycode] : scancode_to_ascii[keycode];
        
        // Handle Ctrl combinations (optional: e.g., Ctrl+C -> 0x03)
        if (ctrl_pressed && key_ascii >= 'a' && key_ascii <= 'z') {
            key_ascii = key_ascii - 'a' + 1;  // Ctrl+key -> ASCII control code
        }

        if (key_ascii) {
            enqueue(&keyboard_queue, (void*)(uintptr_t)key_ascii);
        }
    }

eoi:
    // Send EOI to PIC
    pic_send_eoi(KBD_IRQ);
}

void kbd_init() {
    queue_init(&keyboard_queue);

    // NOTE: pic_init() is called in hal_init(), not here
    
    // Register keyboard interrupt handler
    i686_isr_handler_register(KBD_INT, keyboard_isr_handler);
    
    // Enable keyboard interrupt (IRQ1)
    pic_unmask_irq(KBD_IRQ);
}

char kbd_char_get() { 
    return (char)(uintptr_t)dequeue(&keyboard_queue); 
}
