#include <snow.h>
#include <string.h>

int main() {
	snow_sleep(2000);
	window_t* win = snow_open_window("A static window", 800, 120, WM_NORMAL);

	snow_draw_window(win); // Draws the title bar and borders
	snow_draw_string(win->fb, "Lorem Ipsum", 45, 55, 0x00AA1100);
	snow_draw_border(win->fb, 40, 50, strlen("Lorem Ipsum")*8+10, 26, 0x00);

	snow_render_window(win);

	while (true) {

	}

	snow_close_window(win);

	return 0;
}