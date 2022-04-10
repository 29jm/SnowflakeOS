#include <ui.h>

#include <string.h>
#include <stdlib.h>

#define MAX_WIN 30

ui_app_t make_win() {
    ui_app_t app = ui_app_new("Congrats!", 200, 120, NULL);

    button_t* btn = button_new("You won a prize!");
    btn->widget.flags |= UI_EXPAND;

    ui_set_root(app, W(btn));

    ui_draw(app);

    return app;
}

int main() {
    ui_app_t wins[MAX_WIN];
    int c_win = 0;

    for (int i = 0; i < MAX_WIN; i++) {
        wins[i] = make_win();
    }

    while (MAX_WIN) {
        ui_app_destroy(wins[c_win]);
        wins[c_win] = make_win();

#if MAX_WIN != 0
        c_win = (c_win + 1) % MAX_WIN;
#endif
    }

    return 0;
}