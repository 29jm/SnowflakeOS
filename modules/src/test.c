#include <snow.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_WIN 3

window_t* make_win(char* title) {
	window_t* win = snow_open_window(title, 250, 120, WM_NORMAL);
	char id_str[5];
	itoa(win->id, id_str, 10);

	snow_draw_window(win); // Draws the title bar and borders
	snow_draw_string(win->fb, id_str, 175, 55, 0x00AA1100);

	snow_render_window(win);

	return win;
}

int main() {
	for (int i = 0; i < MAX_WIN; i++) {
		if (i == 0) {
			make_win("redraw me");
		} else {
			make_win("doom.exe (not responding)");
		}
	}

	while (1) { }

	return 0;
}