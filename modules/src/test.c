#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <snow.h>

int main() {
	window_t* win = snow_create_window("Windows XP Install Wizard", 300, 150);

	for (uint32_t i = 0; i < win->fb.width*win->fb.height; i++) {
		((uint32_t*) win->fb.address)[i] = i | i*512 | i % 512;
	}

	snow_draw_window(win);

	while (true);

	return 0;
}
