#include <snow.h>

#include <stdlib.h>
#include <string.h>

uint32_t snow_wm_open_window(fb_t* fb, uint32_t flags) {
	uint32_t id;

	asm (
		"mov $7, %%eax\n"
		"mov %[fb], %%ecx\n"
		"mov %[flags], %%edx\n"
		"int $0x30\n"
		"mov %%eax, %[win_id]\n"
		: [win_id] "=m" (id)
		: [fb] "r" (fb),
		  [flags] "r" (flags)
		: "%eax", "%ecx");

	return id;
}

/* Returns a window object that can be used to draw things in.
 */
window_t* snow_open_window(char* title, int width, int height, uint32_t flags) {
	window_t* win = (window_t*) malloc(sizeof(window_t));
	uint32_t bpp = 32;

	win->title = strdup(title);
	win->width = width;
	win->height = height;
	win->fb = (fb_t) {
		.address = (uintptr_t) aligned_alloc(0x10, width*height*bpp/8),
		.pitch = width*bpp/8,
		.width = width,
		.height = height,
		.bpp = bpp,
	};

	win->id = snow_wm_open_window(&win->fb, flags);
	win->flags = flags;

	return win;
}

void snow_close_window(window_t* win) {
	asm (
		"mov $8, %%eax\n"
		"mov %[win_id], %%ecx\n"
		"int $0x30\n"
		:: [win_id] "r" (win->id)
		: "%eax", "%ecx");
}

/* Draws the given window and its components to the window's buffer then to
 * the screen.
 */
void snow_draw_window(window_t* win) {
	// background
	snow_draw_rect(win->fb, 0, 0, win->width, win->height, 0x00AAAAAA);
	// title bar
	snow_draw_rect(win->fb, 0, 0, win->width, 20, 0x00222221);
	snow_draw_border(win->fb, 0, 0, win->width, 20, 0x00000000);
	snow_draw_string(win->fb, win->title, 3, 3, 0x00FFFFFF);
	// border of the whole window
	snow_draw_border(win->fb, 0, 0, win->width, win->height, 0x00555555);
}

/* Draws the window's buffer as-is to the screen.
 */
void snow_render_window(window_t* win) {
	asm (
		"mov $9, %%eax\n"
		"mov %[win_id], %%ecx\n"
		"int $0x30\n"
		:: [win_id] "r" (win->id)
		: "%eax", "%ecx");
}