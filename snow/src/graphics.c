#include <snow.h>

#include <stdlib.h>

/* Returns information about the display.
 * Note: does not allocate a framebuffer.
 */
fb_info_t snow_get_fb_info() {
	fb_info_t fb;

	asm (
		"mov $6, %%eax\n"
		"int $0x30\n"
		"mov %%eax, %[pitch]\n"
		"mov %%ebx, %[width]\n"
		"mov %%ecx, %[height]\n"
		"mov %%dl, %[bpp]\n"
		: [pitch] "=r" (fb.pitch),
			[width] "=r" (fb.width),
			[height] "=r" (fb.height),
			[bpp] "=r" (fb.bpp)
		:: "%eax"
	);

	return fb;
}

uint32_t* pixel_offset(fb_info_t fb, uint32_t x, uint32_t y) {
	return (uint32_t*)(fb.address + y*fb.pitch + x*fb.bpp/8);
}

void snow_draw_pixel(fb_info_t fb, int x, int y, uint32_t col) {
	uint32_t* offset = pixel_offset(fb, x, y);
	*offset = col;
}

void snow_draw_rect(fb_info_t fb, int x, int y, int w, int h, uint32_t col) {
	uint32_t* offset = pixel_offset(fb, x, y);

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			offset[j] = col;
		}

		offset = (uintptr_t) offset + fb.pitch;
	}
}

void draw_line_low(fb_info_t fb, int x0, int y0, int x1, int y1, uint32_t col) {
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

void draw_line_high(fb_info_t fb, int x0, int y0, int x1, int y1, uint32_t col) {
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

void draw_line_horizontal(fb_info_t fb, int x0, int x1, int y, uint32_t col) {
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

void draw_line_vertical(fb_info_t fb, int x, int y0, int y1, uint32_t col) {
	if (y0 > y1) {
		int y = y0;
		y0 = y1;
		y1 = y;
	}

	uint32_t* offset = pixel_offset(fb, x, y0);

	for (int i = 0; i < y1 - y0; i++) {
		*offset = col;
		offset = (uintptr_t) offset + fb.pitch;
	}
}

void snow_draw_line(fb_info_t fb, int x0, int y0, int x1, int y1, uint32_t col) {
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

void snow_draw_border(fb_info_t fb, int x, int y, int w, int h, uint32_t col) {
	draw_line_horizontal(fb, x, x + w, y, col);
	draw_line_horizontal(fb, x, x + w, y + h, col);
	draw_line_vertical(fb, x, y, y + h, col);
	draw_line_vertical(fb, x + w, y, y + h, col);
}