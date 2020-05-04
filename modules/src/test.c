#include <snow.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_WIN 10

window_t* make_win() {
	window_t* win = snow_open_window("Congratulations!", 200, 120, WM_NORMAL);
	char id_str[5];
	itoa(win->id, id_str, 10);

	snow_draw_window(win); // Draws the title bar and borders
	snow_draw_string(win->fb, id_str, 175, 55, 0x00AA1100);

	snow_render_window(win);

	return win;
}

int main() {
	for (int i = 0; i < MAX_WIN; i++) {
		 make_win();
	}

	while (1) { }

	return 0;
}
