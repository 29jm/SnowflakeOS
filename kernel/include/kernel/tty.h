#ifndef TTY_H
#define TTY_H

#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

typedef enum {
	COLOR_BLACK,
	COLOR_BLUE,
	COLOR_GREEN,
	COLOR_CYAN,
	COLOR_RED,
	COLOR_MAGENTA,
	COLOR_BROWN,
	COLOR_LIGHT_GREY, // Max background color
	COLOR_DARK_GREY,
	COLOR_LIGHT_BLUE,
	COLOR_LIGHT_GREEN,
	COLOR_LIGHT_CYAN,
	COLOR_LIGHT_RED,
	COLOR_LIGHT_MAGENTA,
	COLOR_LIGHT_BROWN,
	COLOR_WHITE,
} vga_color;

void init_term();
void term_putchar_at(char c, size_t x, size_t y);
void term_setcolor(vga_color fg, vga_color bg);
void term_change_bg_color(vga_color bg);
void term_set_blink(int blink);
void term_scrolldown();
void term_putchar(char c);
void term_write(const char* data, size_t size);
void term_writestring(const char* data);
int term_interpret_ansi(char c);

#endif
