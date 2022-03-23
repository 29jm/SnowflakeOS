#include <snow.h>

#include <stdlib.h>
#include <string.h>

color_scheme_t def = {
    .bg_color = 0x00AAAAAA, // bg color
    .tb_color = 0x00222221, // tb color
    .tb_bcolor = 0x00000000, // tb border color
    .tb_tcolor = 0x00FFFFFF, // tb text color
    .w_bcolor = 0x00555555, // window border
};

uint32_t snow_wm_open_window(fb_t* fb, uint32_t flags) {
    wm_param_open_t param = (wm_param_open_t) {
        .fb = fb,
        .flags = flags
    };

    return syscall2(SYS_WM, WM_CMD_OPEN, (uintptr_t) &param);
}

/* Returns a window object that can be used to draw things in.
 */
window_t* snow_open_window(const char* title, int width, int height, uint32_t flags) {
    window_t* win = (window_t*) malloc(sizeof(window_t));
    uint32_t bpp = 32;

    win->title = strdup(title);
    win->width = width;
    win->height = height;
    win->fb = (fb_t) {
        .address = (uintptr_t) zalloc(width*height*bpp/8),
        .pitch = width*bpp/8,
        .width = width,
        .height = height,
        .bpp = bpp,
    };

    win->id = snow_wm_open_window(&win->fb, flags);
    win->flags = flags;

    return win;
}

void snow_close_window(window_t* win) {
    syscall2(SYS_WM, WM_CMD_CLOSE, win->id);

    free(win->title);
    free((void*) win->fb.address);
    free(win);
}

/* Draws the given window and its components to the window's buffer then to
 * the screen.
 */
void snow_draw_window(window_t* win, color_scheme_t* clr) {

    if (clr == NULL) {
        clr = &def;
    }

    // background
    snow_draw_rect(win->fb, 0, 0, win->width, win->height, clr->bg_color);
    // title bar
    snow_draw_rect(win->fb, 0, 0, win->width, tb_height, clr->tb_color);
    snow_draw_border(win->fb, 0, 0, win->width, tb_height, clr->tb_bcolor);
    snow_draw_string(win->fb, win->title, tb_padding, tb_height / 3, clr->tb_tcolor);
    // border of the whole window
    snow_draw_border(win->fb, 0, 0, win->width, win->height, clr->w_bcolor);
}

/* Draws the window's buffer as-is to the screen.
 */
void snow_render_window(window_t* win) {
    wm_param_render_t param = {
        .win_id = win->id,
        .clip = NULL
    };

    syscall2(SYS_WM, WM_CMD_RENDER, (uintptr_t) &param);
}

void snow_render_window_partial(window_t* win, wm_rect_t clip) {
    wm_param_render_t param = {
        .win_id = win->id,
        .clip = &clip
    };

    syscall2(SYS_WM, WM_CMD_RENDER, (uintptr_t) &param);
}

wm_event_t snow_get_event(window_t* win) {
    wm_event_t event;

    wm_param_event_t param = {
        .win_id = win->id,
        .event = &event
    };

    syscall2(SYS_WM, WM_CMD_EVENT, (uintptr_t) &param);

    return event;
}