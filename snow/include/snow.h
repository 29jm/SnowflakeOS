#ifndef SNOWLIB_H
#define SNOWLIB_H

#include <stdint.h>

typedef struct {
	uintptr_t address;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint8_t bpp;
} fb_info_t;

typedef struct {
	char* title;
	uint32_t width;
	uint32_t height;
	uint32_t x;
	uint32_t y;
	fb_info_t fb;
} window_t;

void* snow_alloc(uint32_t size);
fb_info_t snow_get_fb_info();

// Drawing functions
void snow_render(uintptr_t buffer);
void snow_draw_pixel(fb_info_t fb, int x, int y, uint32_t col);
void snow_draw_rect(fb_info_t fb, int x, int y, int w, int h, uint32_t col);
void snow_draw_line(fb_info_t fb, int x0, int y0, int x1, int y1, uint32_t col);
void snow_draw_border(fb_info_t fb, int x, int y, int w, int h, uint32_t col);

// GUI functions
window_t snow_create_window(char* title, int width, int height);

#endif