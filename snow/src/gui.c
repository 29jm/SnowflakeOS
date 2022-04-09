#include <snow.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Draws the given window and its components to the window's buffer.
 */
void snow_draw_window(window_t* win) {
    // TODO: eliminate this whole function: defer the drawing of decorations
    // to the UI library only.
    uint32_t bg_color = 0x353535;
    uint32_t base_color = 0x222221;
    uint32_t highlight = 0x030303;
    uint32_t border_color = 0x000000;
    uint32_t text_color = 0xFFFFFF;
    uint32_t border_color2 = 0x121212;

    // Figure out if we need to highlight the title bar
    bool titlebar_hovered = syscall2(SYS_WM, WM_CMD_IS_HOVERED, win->id);

    // background
    snow_draw_rect(win->fb, 0, 0, win->width, win->height, bg_color);

    // title bar
    if (titlebar_hovered) {
        snow_draw_rect(win->fb, 0, 0, win->width, WM_TB_HEIGHT, base_color + highlight);
        snow_draw_border(win->fb, 0, 0, win->width, WM_TB_HEIGHT, highlight);
    } else {
        snow_draw_rect(win->fb, 0, 0, win->width, WM_TB_HEIGHT, base_color);
        snow_draw_border(win->fb, 0, 0, win->width, WM_TB_HEIGHT, border_color);
    }

    // window title + borders
    snow_draw_string(win->fb, win->title, 8, WM_TB_HEIGHT / 3, text_color);
    snow_draw_border(win->fb, 0, 0, win->width, win->height, border_color2);
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