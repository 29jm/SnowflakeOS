#include <kernel/term.h>
#include <kernel/ansi_interpreter.h>
#include <kernel/paging.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <stdio.h>

#define ENTRY(x, y) term_buffer[(y)*TERM_WIDTH+(x)]

static ansi_interpreter_context ctx;
static uint32_t term_row;
static uint32_t term_column;
static uint8_t term_color;
static uint16_t* term_buffer;

// Helper functions
static uint8_t term_make_color(term_color_t fg, term_color_t bg) {
	return fg | bg << 4;
}

// Entry format: [ BLINK BG BG BG FG FG FG FG | C C C C C C C C ]
//               [           8-bits           |      8-bits     ]
static uint16_t term_make_entry(char c, uint8_t color) {
	uint16_t c16 = c;
	uint16_t color16 = color;
	return c16 | color16 << 8;
}

// Terminal handling functions
void init_term() {
	term_color = term_make_color(TERM_COLOR_WHITE, TERM_COLOR_BLACK);
	term_buffer = (uint16_t*) TERM_MEMORY;
	term_row = 0;
	term_column = 0;

	uint16_t entry = term_make_entry(' ', term_color);

	for (uint32_t x = 0; x < TERM_WIDTH; x++) {
		for (uint32_t y = 0; y < TERM_HEIGHT; y++) {
			ENTRY(x, y) = entry;
		}
	}

	ansi_init_context(&ctx);

	// Remap the terminal's buffer in the kernel heap.
	// We allocate a full page as we're going to modify it manually
	// Note that `size` is 4000 bytes, a page is 4096 bytes
	uintptr_t buff = (uintptr_t) kamalloc(0x1000, 0x1000);
	page_t* p = paging_get_page((uintptr_t) buff, false, 0);
	*p = TERM_MEMORY | PAGE_PRESENT | PAGE_RW;
	term_buffer = (uint16_t*) buff;
}

void term_change_bg_color(term_color_t bg) {
	for (uint32_t x = 0; x < TERM_WIDTH; x++) {
		for (uint32_t y = 0; y < TERM_HEIGHT; y++) {
			uint16_t entry = ENTRY(x, y);
			char c = entry & 0xFF;
			term_color_t fg = (entry & 0x0F00) >> 8;
			ENTRY(x, y) = term_make_entry(c, term_make_color(fg, bg));
		}
	}

	term_set_bg_color(bg);
}

void term_set_blink(int blink) {
	if (blink) {
		term_color |= (1 << 7);
	}
	else {
		term_color &= ~(1 << 7);
	}
}

void term_scrolldown() {
	for (uint32_t y = 0; y < TERM_HEIGHT; y++) {
		for (uint32_t x = 0; x < TERM_WIDTH; x++) {
			if (y < TERM_HEIGHT-1) {
				ENTRY(x, y) = ENTRY(x, y+1);
			} else { // last line
				ENTRY(x, y) = term_make_entry(' ', term_color);
			}
		}
	}

	term_row--;
}

void term_new_line() {
	for (int i = term_column; i < TERM_WIDTH; i++) {
		term_putchar(' ');
	}
}

void term_putchar_at(char c, uint32_t x, uint32_t y) {
	if (y >= TERM_HEIGHT || x >= TERM_WIDTH) {
		return;
	}

	ENTRY(x, y) = term_make_entry(c, term_color);
}

void term_putchar(char c) {
	if (c == '\n' || c == '\r') {
		term_new_line();
	} else if (c == '\t') {
		for (int i = 0; i < 4; i++) {
			term_putchar_at(' ', term_column++, term_row);
		}
	}

	if (term_column >= TERM_WIDTH) {
		term_row += 1;
		term_column = 0;
	}

	if (term_row >= TERM_HEIGHT) {
		term_scrolldown();
	}

	if (ansi_interpret_char(&ctx, c) || !isprint(c)) {
		return;
	}

	term_putchar_at(c, term_column++, term_row);
}

void term_write_string(const uint8_t* data) {
	for (uint32_t i = 0; i < strlen((char*) data); i++) {
		term_putchar(data[i]);
	}
}

// Getters
uint32_t term_get_row() {
	return term_row;
}

uint32_t term_get_column() {
	return term_column;
}

uint8_t term_get_color() {
	return term_color;
}

uint8_t term_get_fg_color() {
	return term_color & 0x000F;
}

uint8_t term_get_bg_color() {
	return (term_color >> 4) & 0x07;
}

uint16_t* term_get_buffer() {
	return term_buffer;
}

// Setters
void term_set_row(uint32_t row) {
	term_row = row;
}

void term_set_column(uint32_t column) {
	term_column = column;
}

void term_set_color(uint8_t color) {
	term_color = color;
}

void term_set_bg_color(term_color_t color) {
	term_color = term_make_color(term_get_fg_color(), color);
}

void term_set_fg_color(term_color_t color) {
	term_color = term_make_color(color, term_get_bg_color());
}

void term_set_buffer(uint16_t* buffer) {
	term_buffer = buffer;
}
