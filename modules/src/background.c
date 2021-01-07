#include <snow.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main() {
    fb_t scr;
    char time_text[20] = "uptime: ";
    snow_get_fb_info(&scr);

    window_t* win = snow_open_window("bg", scr.width, scr.height, WM_BACKGROUND);

    FILE* wp = fopen("/wallpaper.rgb", "r");

    if (wp) {
        uint32_t n_pixels = win->fb.width*win->fb.height;
        uint8_t* bg = malloc(n_pixels * 3);
        fread(bg, 3, n_pixels, wp);
        snow_draw_rgb(win->fb, bg, 0, 0, win->fb.width, win->fb.height);
        free(bg);
        fclose(wp);
    } else {
        snow_draw_rect(win->fb, 0, 0, scr.width, scr.height, 0x9AC4F8);
    }

    snow_draw_rect(win->fb, 0, 0, win->fb.width, 22, 0x303030);
    snow_draw_string(win->fb, "Snowflake OS 0.7", 3, 3, 0x00FFFFFF);
    snow_render_window(win);

    while (true) {
        wm_event_t evt = snow_get_event(win);

        if (evt.type == WM_EVENT_KBD && evt.kbd.keycode == KBD_T) {
            syscall2(SYS_EXEC, (uintptr_t) "terminal", (uintptr_t) NULL);
        }

        sys_info_t info;
        syscall2(SYS_INFO, SYS_INFO_UPTIME, (uintptr_t) &info);

        uint32_t time = info.uptime;
        uint32_t m = time / 60;
        uint32_t s = time % 60;
        itoa(m, time_text+8, 10);
        strcpy(time_text+strlen(time_text), "m");
        itoa(s, time_text + strlen(time_text), 10);
        strcpy(time_text+strlen(time_text), "s");
        int x = win->fb.width / 2 - strlen(time_text)*8/2;
        int y = 3;

        wm_rect_t redraw = { // Redraw one character too much
            .left = x, .top = y, .bottom = y+16, .right = x+strlen(time_text)*8+8
        };

        snow_draw_rect(win->fb, 0, 0, win->fb.width, 22, 0x303030);
        snow_draw_string(win->fb, time_text, x, y, 0xFFFFFF);
        snow_render_window_partial(win, redraw);

        snow_sleep(500);
    }

    snow_close_window(win);

    return 0;
}