#include <snow.h>

#include <string.h>
#include <stdlib.h>

#define MAX_WIN 30

window_t* make_win() {
    window_t* win = snow_open_window("Congratulations!", 200, 120, WM_NORMAL);

    snow_draw_window(win); // Draws the title bar and borders
    snow_draw_string(win->fb, "You won a prize!", 45, 55, 0x00AA1100);
    snow_draw_border(win->fb, 40, 50, strlen("You won a prize!")*8+10, 26, 0x00);

    snow_render_window(win);

    return win;
}

int main() {
    window_t* wins[MAX_WIN];
    int c_win = 0;

    for (int i = 0; i < MAX_WIN; i++) {
        wins[i] = make_win();
    }

    while (MAX_WIN) {
        snow_close_window(wins[c_win]);
        wins[c_win] = make_win();

#if MAX_WIN != 0
        c_win = (c_win + 1) % MAX_WIN;
#endif
    }

    return 0;
}