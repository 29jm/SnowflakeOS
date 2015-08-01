#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <kernel/tty.h>

#define ENTRY(x, y) term_buffer[(y)*VGA_WIDTH+(x)]

static uint32_t term_row;
static uint32_t term_column;
static uint8_t term_color;
static uint16_t* term_buffer;

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

void term_set_blink(int blink) {
	if (blink) {
		term_color |= (1 << 7);
	}
	else {
		term_color &= ~(1 << 7);
	}
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

	if (term_interpret_ansi(c) || !isprint(c)) {
		return;
	}

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

// Returns != 0 if in an ANSI sequence
// ANSI escape sequence: \x1B[param;param2...end_char
int term_interpret_ansi(char c) {
	typedef enum {
		normal, bracket, params
	} state_t;

	static state_t state = normal;     // State tracker
	static char buf[32] = "";          // Stores the string for 1 param
	static uint32_t args[32] = { 0 };  // Stores the parsed params
	static uint32_t current_arg = 0;   // Number of params
	static uint32_t current_index = 0; // Number of chars in current param
	static uint32_t saved_row = 0;
	static uint32_t saved_col = 0;

	if (state == normal) {
		if (c == 0x1B) { // Escape character
			state = bracket;
		}
		else {
			return 0;
		}
	}
	else if (state == bracket) {
		if (c == '[') {
			state = params;
		}
		else {
			state = normal;
			return 0;
		}
	}
	else if (state == params) {
		if (c == ';') {
			buf[current_index] = '\0';
			args[current_arg++] = atoi(buf);
			current_index = 0;
		}
		else if (isdigit(c)) {
			if (current_index >= 32) {
				current_arg = 0;
				current_index = 0;
				state = normal;
			}
			else {
				buf[current_index++] = c;
			}
		}
		else if (isalpha(c)) {
			buf[current_index] = '\0';
			args[current_arg++] = atoi(buf);

			switch (c) {
				case 's': // Save cursor position
					saved_row = term_row;
					saved_col = term_column;
					state = normal;
					break;
				case 'u': // Restore cursor position
					term_row = saved_row;
					term_column = saved_col;
					state = normal;
					break;
				case 'K': // Erase until the end of line
					for (uint32_t x = term_column; x < VGA_WIDTH; x++) {
						ENTRY(x, term_row) = term_make_entry(' ', term_color);
					}
					state = normal;
					break;
				case 'H': // Set cursor position
				case 'f':
					term_row = args[0];
					term_column = args[1];
					break;
				case 'A': // Cursor up
					term_row -= args[0]; break;
				case 'B': // Cursor down
					term_row += args[0]; break;
				case 'C': // Cursor right
					term_column += args[0]; break;
				case 'D': // Cursor left
					term_column -= args[0]; break;
				case 'J': // 2J: clear screen & reset cursor
					if (args[0] == 2) {
						init_term();
					}
					break;
			}

			if (c == 'm') { // Set graphics mode
				for (uint32_t i = 0; i < current_arg; i++) {
					switch (args[i]) {
						case 0:
							term_set_blink(0); break;
						case 1: break;
						case 4: break;
						case 5:
							term_set_blink(1); break;
						case 30:
							term_set_fg_color(COLOR_BLACK); break;
						case 31:
							term_set_fg_color(COLOR_RED); break;
						case 32:
							term_set_fg_color(COLOR_GREEN); break;
						case 33: // Yellow
							term_set_fg_color(COLOR_BROWN); break;
						case 34:
							term_set_fg_color(COLOR_BLUE); break;
						case 35:
							term_set_fg_color(COLOR_MAGENTA); break;
						case 36:
							term_set_fg_color(COLOR_CYAN); break;
						case 37:
							term_set_fg_color(COLOR_WHITE); break;
						case 40:
							term_set_bg_color(COLOR_BLACK); break;
						case 41:
							term_set_bg_color(COLOR_RED); break;
						case 42:
							term_set_bg_color(COLOR_GREEN); break;
						case 43: // Yellow
							term_set_bg_color(COLOR_BROWN); break;
						case 44:
							term_set_bg_color(COLOR_BLUE); break;
						case 45:
							term_set_bg_color(COLOR_MAGENTA); break;
						case 46:
							term_set_bg_color(COLOR_CYAN); break;
						case 47: // White
							term_set_bg_color(COLOR_LIGHT_GREY); break;
					}
				}
			}

			current_arg = 0;
			current_index = 0;
			state = normal;
		}
	}

	return 1;
}
