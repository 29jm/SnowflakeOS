#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <kernel/tty.h>

#define ENTRY(x, y) term_buffer[(y)*VGA_WIDTH+(x)]

uint32_t term_row;
uint32_t term_column;
uint8_t term_color;
uint16_t* term_buffer;

// Helper functions
static uint8_t term_make_color(vga_color fg, vga_color bg) {
	return fg | bg << 4;
}

// Entry format: [ BLINK BG BG BG FG FG FG FG | C C C C C C C C ]
//               [           8-bits           |      8-bits     ]
static uint16_t term_make_entry(char c, uint8_t color) {
	uint16_t c16 = c;
	uint16_t color16 = color;
	return c16 | color16 << 8;
}

static uint8_t term_get_fg_color() {
	return term_color & 0x000F;
}

static uint8_t term_get_bg_color() {
	return term_color >> 4;
}

static void term_set_bg_color(vga_color color) {
	term_color = term_make_color(term_get_fg_color(), color);
}

static void term_set_fg_color(vga_color color) {
	term_color = term_make_color(color, term_get_bg_color());
}

// Terminal handling functions
void init_term() {
	uint16_t entry = term_make_entry(' ', term_color);
	term_buffer = (uint16_t*) VGA_MEMORY;
	term_color = term_make_color(COLOR_WHITE, COLOR_BLACK);
	term_row = 0;
	term_column = 0;

	for (uint32_t x = 0; x < VGA_WIDTH; x++) {
		for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
			ENTRY(x, y) = entry;
		}
	}
}

void term_setcolor(vga_color fg, vga_color bg) {
	term_color = term_make_color(fg, bg);
}

void term_change_bg_color(vga_color bg) {
	for (uint32_t x = 0; x < VGA_WIDTH; x++) {
		for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
			uint16_t entry = ENTRY(x, y);
			char c = entry & 0xFF;
			vga_color fg = (entry & 0x0F00) >> 8;
			ENTRY(x, y) = term_make_entry(c, term_make_color(fg, bg));
		}
	}

	term_set_bg_color(bg);
}

void term_putchar_at(char c, uint32_t x, uint32_t y) {
	if (y >= VGA_HEIGHT || x >= VGA_WIDTH) {
		return;
	}

	ENTRY(x, y) = term_make_entry(c, term_color);
}

void term_scrolldown() {
	for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
		for (uint32_t x = 0; x < VGA_WIDTH; x++) {
			if (y < VGA_HEIGHT-1) {
				ENTRY(x, y) = ENTRY(x, y+1);
			}
			else { // last line
				ENTRY(x, y) = term_make_entry(' ', term_color);
			}
		}
	}

	term_row--;
}

void term_putchar(char c) {
	if (c == '\n' || c == '\r') {
		term_row += 1;
		term_column = 0;
	}
	else if (c == '\t') {
		for (int i = 0; i < 4; i++) {
			term_putchar_at(' ', term_column++, term_row);
		}
	}

	if (term_column >= VGA_WIDTH) {
		term_row += 1;
		term_column = 0;
	}

	if (term_row >= VGA_HEIGHT) {
		term_scrolldown();
	}

	if (!isprint(c))
		return;

	term_putchar_at(c, term_column++, term_row);
}

void term_write(const char* data, uint32_t size) {
	for (uint32_t i = 0; i < size; i++) {
		term_putchar(data[i]);
	}
}

void term_write_string(const char* data) {
	term_write(data, strlen(data));
}
