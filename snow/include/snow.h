#ifndef SNOWLIB_H
#define SNOWLIB_H

#include <stdint.h>
#include <stdbool.h>

// Sets a magic breakpoint in Bochs on the line it's called.
#define BREAK() do { \
                	asm ("xchgw %bx, %bx\n"); \
                } while (false)

typedef struct {
	uintptr_t address;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint8_t bpp;
} fb_t;

typedef struct {
	char* title;
	uint32_t width;
	uint32_t height;
	fb_t fb;
	uint32_t id;
} window_t;

void* snow_alloc(uint32_t n);
fb_t snow_get_fb_info();

// Drawing functions
void snow_draw_pixel(fb_t fb, int x, int y, uint32_t col);
void snow_draw_rect(fb_t fb, int x, int y, int w, int h, uint32_t col);
void snow_draw_line(fb_t fb, int x0, int y0, int x1, int y1, uint32_t col);
void snow_draw_border(fb_t fb, int x, int y, int w, int h, uint32_t col);
void snow_draw_character(fb_t fb, char c, int x, int y, uint32_t col);
void snow_draw_string(fb_t fb, char* str, int x, int y, uint32_t col);

// GUI functions
window_t* snow_open_window(char* title, int width, int height);
void snow_close_window(window_t* win);
void snow_draw_window(window_t* win);
void snow_render_window(window_t* win);

#endif