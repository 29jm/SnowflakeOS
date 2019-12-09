#include <snow.h>
#include <font.h> // TODO: move to kernel, ask the kernel to map into process

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Helper functions */

uint32_t* pixel_offset(fb_t fb, uint32_t x, uint32_t y) {
	return (uint32_t*) (fb.address + y*fb.pitch + x*fb.bpp/8);
}

void draw_line_low(fb_t fb, int x0, int y0, int x1, int y1, uint32_t col) {
	int dx = x1 - x0;
	int dy = y1 - y0;
	int yi = 1;

	if (dy < 0) {
		yi = -1;
		dy = -dy;
	}

	int D = 2*dy - dx;
	int y = y0;

	for (int x = x0; x < x1; x++) {
		snow_draw_pixel(fb, x, y, col);

		if (D > 0) {
			y += yi;
			D -= 2*dx;
		}

		D += 2*dy;
	}
}

void draw_line_high(fb_t fb, int x0, int y0, int x1, int y1, uint32_t col) {
	int dx = x1 - x0;
	int dy = y1 - y0;
	int xi = 1;

	if (dx < 0) {
		xi = -1;
		dx = -dx;
	}

	int D = 2*dx - dy;
	int x = x0;

	for (int y = y0; y < y1; y++) {
		snow_draw_pixel(fb, x, y, col);

		if (D > 0) {
			x += xi;
			D -= 2*dy;
		}

		D += 2*dx;
	}
}

void draw_line_horizontal(fb_t fb, int x0, int x1, int y, uint32_t col) {
	if (x0 > x1) {
		int x = x0;
		x0 = x1;
		x1 = x;
	}

	uint32_t* offset = pixel_offset(fb, x0, y);

	for (int i = 0; i < x1 - x0; i++) {
		offset[i] = col;
	}
}

void draw_line_vertical(fb_t fb, int x, int y0, int y1, uint32_t col) {
	if (y0 > y1) {
		int y = y0;
		y0 = y1;
		y1 = y;
	}

	uint32_t* offset = pixel_offset(fb, x, y0);

	for (int i = 0; i < y1 - y0; i++) {
		*offset = col;
		offset = (uint32_t*)((uintptr_t) offset + fb.pitch);
	}
}

bool is_within(fb_t fb, int x, int y) {
	return x >= 0 && x < (int) fb.width && y >= 0 && y < (int) fb.height;
}

/* Exposed drawing functions. TODO: make sure not to draw outside the buffer */

void snow_draw_pixel(fb_t fb, int x, int y, uint32_t col) {
	uint32_t* offset = pixel_offset(fb, x, y);
	*offset = col;
}

void snow_draw_rect(fb_t fb, int x, int y, int w, int h, uint32_t col) {
	uint32_t* offset = pixel_offset(fb, x, y);

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			offset[j] = col;
		}

		offset = (uint32_t*)((uintptr_t) offset + fb.pitch);
	}
}

void snow_draw_line(fb_t fb, int x0, int y0, int x1, int y1, uint32_t col) {
	if (y0 == y1) {
		draw_line_horizontal(fb, x0, x1, y0, col);
		return;
	}

	if (x0 == x1) {
		draw_line_vertical(fb, x0, y0, y1, col);
	}

	if (abs(y1 - y0) < abs(x1 - x0)) {
		if (x0 > x1) {
			draw_line_low(fb, x1, y1, x0, y0, col);
		} else {
			draw_line_low(fb, x0, y0, x1, y1, col);
		}
	} else {
		if (y0 > y1) {
			draw_line_high(fb, x1, y1, x0, y0, col);
		} else {
			draw_line_high(fb, x0, y0, x1, y1, col);
		}
	}
}

void snow_draw_border(fb_t fb, int x, int y, int w, int h, uint32_t col) {
	draw_line_horizontal(fb, x, x + w, y, col);
	draw_line_horizontal(fb, x, x + w, y + h, col);
	draw_line_vertical(fb, x, y, y + h, col);
	draw_line_vertical(fb, x + w, y, y + h, col);
}

/* Font stuff */

/* Draws a character from its top left corner at coordinates (x, y).
 * Does not draw the background.
 */
void snow_draw_character(fb_t fb, char c, int x, int y, uint32_t col) {
	uint8_t* offset = font_psf + sizeof(font_header_t) + 16*c;

	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 8; j++) {
			if (offset[i] & (1 << j)) {
				snow_draw_pixel(fb, x + 8 - j, y + i, col);
			}
		}
	}
}

void snow_draw_string(fb_t fb, char* str, int x, int y, uint32_t col) {
	for (size_t i = 0; i < strlen(str); i++) {
		snow_draw_character(fb, str[i], x + 8*i, y, col);
	}
}