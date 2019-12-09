#include <snow.h>

#include <stdlib.h>
#include <stdio.h>

window_t* snow_create_window(char* title, int width, int height) {
	window_t* win = (window_t*) malloc(sizeof(window_t));
	win->title = title;
	win->width = width;
	win->height = height;
	win->fb = snow_get_fb_info();
	win->fb.address = (uintptr_t) aligned_alloc(0x10, win->fb.pitch*win->fb.height);
	win->x = win->fb.width/2 - width/2;
	win->y = win->fb.height/2 - height/2;

	// TODO: register that window with something
	return win;
}

/* Copies the given window to the associated framebuffer and displays it on
 * the screen.
 * TODO: only copy, then syscall to tell the compositor that this framebuffer
 * is ready to be composited.
 */
void snow_draw_window(window_t* win) {
	// background
	snow_draw_rect(win->fb, win->x, win->y, win->width, win->height, 0x00AAAAAA);
	// title bar
	snow_draw_rect(win->fb, win->x, win->y, win->width, 20, 0x000000FF);
	snow_draw_border(win->fb, win->x, win->y, win->width, 20, 0x00000000);
	snow_draw_string(win->fb, win->title, win->x+3, win->y+3, 0x00FFFFFF);
	// exit button
	snow_draw_rect(win->fb, win->x+win->width-17, win->y+3, 15, 15, 0x00FF0000);
	snow_draw_border(win->fb, win->x+win->width-17, win->y+3, 15, 15, 0x00000000);
	// border of the whole window
	snow_draw_border(win->fb, win->x, win->y, win->width, win->height, 0x00000000);

	snow_render(win->fb);
}