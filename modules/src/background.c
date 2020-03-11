#include <snow.h>

#include <stdio.h>

#define RELEASE 0

#if RELEASE == 1
#include "bg.h"
#endif

int main() {
	fb_t scr;
	snow_get_fb_info(&scr);

	window_t* win = snow_open_window("bg", scr.width, scr.height, WM_BACKGROUND);

#if RELEASE == 1
	uint32_t j = 0;

	for (uint32_t i = 0; i < win->fb.width*win->fb.height; i++) {
		uint32_t px = bg[j] << 16 | bg[j+1] << 8 | bg[j+2];
		((uint32_t*) win->fb.address)[i] = px;
		j += 3;
	}
#else
	for (uint32_t i = 0; i < win->fb.height*win->fb.width; i++) {
		((uint32_t*) win->fb.address)[i] = i | i % 512 | i * 512;
	}
#endif

	snow_draw_string(win->fb, "Snowflake OS 0.3", 3, 3, 0x00FFFFFF);

	snow_render_window(win);

	while (true) {

	}

	snow_close_window(win);

	return 0;
}