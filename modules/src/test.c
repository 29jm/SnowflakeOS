#include <snow.h>

#include <string.h>
#include <stdlib.h>

#define MAX_WIN 20

window_t* make_win() {
	window_t* win = snow_open_window("A static window", 200, 120, WM_NORMAL);

	snow_draw_window(win); // Draws the title bar and borders
	snow_draw_string(win->fb, "Lorem Ipsum", 45, 55, 0x00AA1100);
	snow_draw_border(win->fb, 40, 50, strlen("Lorem Ipsum")*8+10, 26, 0x00);

	snow_render_window(win);
	snow_sleep(500);

	return win;
}

int main() {
	window_t* wins[MAX_WIN];
	int c_win = 0;

	for (int i = 0; i < MAX_WIN; i++) {
		wins[i] = make_win();
	}

	while (true) {
		snow_close_window(wins[c_win]);
		wins[c_win] = make_win();
		c_win = (c_win + 1) % MAX_WIN;
	}

	return 0;
}