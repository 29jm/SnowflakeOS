#include <snow.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define RELEASE 0

#if RELEASE == 1
#include "bg.h"
#endif

int main() {
	fb_t scr;
	char time_text[20] = "uptime: ";
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

	snow_draw_rect(win->fb, 0, 0, win->fb.width, 22, 0x303030);
	snow_draw_string(win->fb, "Snowflake OS 0.4", 3, 3, 0x00FFFFFF);
	snow_render_window(win);

	while (true) {
		sys_info_t info;
		syscall2(SYS_INFO, SYS_INFO_UPTIME, (uintptr_t) &info);

		uint32_t time = info.uptime;
		itoa(time, time_text+8, 10);
		int x = win->fb.width / 2 - strlen(time_text)*8/2;
		int y = 3;

		rect_t redraw = {
			.left = x, .top = y, .bottom = y+16, .right = x+strlen(time_text)*8
		};

		snow_draw_rect(win->fb, 0, 0, win->fb.width, 22, 0x303030);
		snow_draw_string(win->fb, "Snowflake OS 0.4", 3, 3, 0x00FFFFFF);
		snow_draw_string(win->fb, time_text, x, y, 0xFFFFFF);
		snow_render_window_partial(win, redraw);

		snow_sleep(500);
	}

	snow_close_window(win);

	return 0;
}