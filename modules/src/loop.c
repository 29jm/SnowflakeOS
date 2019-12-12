#include <snow.h>

int main() {
	fb_t screen = snow_get_fb_info();
	window_t* win = snow_open_window("background", screen.width, screen.height);

	for (uint32_t i = 0; i < win->fb.width*win->fb.height; i++) {
		((uint32_t*) win->fb.address)[i] = i | i*512 | i % 512;
	}

	while (true) {
		snow_render_window(win);
	}

	snow_close_window(win);

	return 0;
}