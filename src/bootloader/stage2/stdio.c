#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000
static uint16_t* vga_buffer = (uint16_t*)VGA_ADDRESS;
static int cursor_row = 0;
static int cursor_col = 0;

void clear_screen() {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_ADDRESS;
    for (int i = 0; i < VGA_WIDTH * 25; i++) {
        vga[i] = 0x0F20; // Space with white on black
    }
}
static void put_char(char c, uint8_t color) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        return;
    }

    vga_buffer[cursor_row * VGA_WIDTH + cursor_col] =
        ((uint16_t)color << 8) | c;
    cursor_col++;
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
    }
}

// Simple integer printing
static void print_int(int n, uint8_t color) {
    char buf[12];
    int i = 0;
    int is_negative = 0;

    if (n == 0) {
        put_char('0', color);
        return;
    }

    if (n < 0) {
        is_negative = 1;
        n = -n;
    }

    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }

    if (is_negative)
        buf[i++] = '-';

    while (i--)
        put_char(buf[i], color);
}

// Simple integer printing
static void print_uint(int u, uint8_t color) {
    char buf[12];
    int i = 0;

    if (u == 0) {
        put_char('0', color);
        return;
    }

    while (u > 0) {
        buf[i++] = '0' + (u % 10);
        u /= 10;
    }

    while (i--)
        put_char(buf[i], color);
}

static void print_hex(uint32_t n, uint8_t color, bool upper) {
    char buf[10];
    int i = 0;

    if (n == 0) {
        put_char('0', color);
        return;
    }

    while (n > 0) {
        int dig_val = (n % 0x10);
        if (dig_val < 0xA) {
            buf[i++] = '0' + dig_val;
        } else {
            buf[i++] = (upper ? 'A' : 'a') + (dig_val - 0xA);
        }
        n /= 0x10;
    }

    // print "0x" prefix
    put_char('0', color);
    put_char('x', color);

    while (i--)
        put_char(buf[i], color);
}

// Simple string printing
static void print_string(const char* s, uint8_t color) {
    while (*s) {
        put_char(*s++, color);
    }
}

// Very basic printf
void printf(const char* fmt, ...) {
    uint8_t color = 0x07; // light grey on black
    const char* p = fmt;
    va_list args;
    va_start(args, fmt);

    for (; *p; p++) {
        if (*p == '%') {
            p++;
            switch (*p) {
            case 'd':
                print_int(va_arg(args, int), color);
                break;
            case 'u':
                print_uint(va_arg(args, uint32_t), color);
                break;
            case 'x':
                print_hex(va_arg(args, uint32_t), color, false);
                break;
            case 'X':
                print_hex(va_arg(args, uint32_t), color, true);
                break;
            case 's':
                print_string(va_arg(args, const char*), color);
                break;
            case 'c':
                put_char((char)va_arg(args, int), color);
                break;
            case '%':
                put_char('%', color);
                break;
                ;
                break;
            default:
                put_char(*p, color);
            }
        } else {
            put_char(*p, color);
        }
    }

    va_end(args);
}
