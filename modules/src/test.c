#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <snow.h>

int main() {
	window_t* win = snow_open_window("SnowflakeOS Installer", 300, 150);
	window_t* win2 = snow_open_window("Test window", 300, 100);

	while (true) {
		snow_draw_window(win);
		snow_draw_window(win2);
	}

	snow_close_window(win);
	snow_close_window(win2);

	return 0;
}