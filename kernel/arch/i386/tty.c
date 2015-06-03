#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/vga.h>

#ifdef YCM
#define size_t uint32_t
#endif

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_reset() {
	uint16_t entry = make_vga_entry(' ', terminal_color);
	terminal_row = 0;
	terminal_column = 0;

	for (size_t x = 0; x < VGA_WIDTH; x++) {
		for (size_t y = 0; y < VGA_HEIGHT; y++) {
			terminal_buffer[y * VGA_WIDTH + x] = entry;
		}
	}
}

void terminal_init() {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = make_color(COLOR_LIGHT_GREY, COLOR_DARK_GREY);
	terminal_buffer = VGA_MEMORY;
	terminal_reset();
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

void terminal_putchar_at(char c, size_t x, size_t y) {
	if (y >= VGA_HEIGHT || x >= VGA_WIDTH) {
		return;
	}

	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = make_vga_entry(c, terminal_color);
}

void terminal_scrolldown() {
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y*VGA_WIDTH+x;
			const size_t down_index = (y+1)*VGA_WIDTH+x;

			if (y < VGA_HEIGHT-1) {
				terminal_buffer[index] = terminal_buffer[down_index];
			}
			else { // last line
				terminal_buffer[index] = make_vga_entry(' ', terminal_color);
			}
		}
	}

	terminal_row--;
}

void terminal_putchar(const char character) {
	if (character == '\n') {
		terminal_row += 1;
		terminal_column = 0;
	}
	else if (character == '\t') {
		for (int i = 0; i < 4; i++)
			terminal_putchar_at(' ', terminal_column++, terminal_row);
	}

	if (terminal_column >= VGA_WIDTH) {
		terminal_row += 1;
		terminal_column = 0;
	}

	if (terminal_row >= VGA_HEIGHT) {
		terminal_scrolldown();
	}

	// Non-printable
	if (character == '\n' || character == '\t')
		return;

	terminal_putchar_at(character, terminal_column++, terminal_row);
}

void terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_write_string(const char* data) {
	terminal_write(data, strlen(data));
}
