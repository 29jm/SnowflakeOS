#pragma once

#include <stdbool.h>

#define WM_NORMAL 0
#define WM_BACKGROUND 1
#define WM_FOREGROUND 2
#define WM_SKIP_INPUT 4

// UI cross-section metadata
#define UI_TB_HEIGHT 30
#define UI_TB_PADDING 8

enum WM_CMD {
    WM_CMD_OPEN,
    WM_CMD_CLOSE,
    WM_CMD_RENDER,
    WM_CMD_INFO,
    WM_CMD_EVENT,
    WM_CMD_GET_POS,
    WM_CMD_IS_DRAGGED,
    WM_CMD_IS_HOVERED,
};

enum WM_EVENT {
    WM_EVENT_CLICK = 1,
    WM_EVENT_MOUSE_MOVE,
    WM_EVENT_MOUSE_RELEASE,
    WM_EVENT_KBD,
    WM_EVENT_GAINED_FOCUS,
    WM_EVENT_LOST_FOCUS,
};

typedef struct {
    int32_t x, y;
} point_t;

typedef struct {
    int32_t top, left, bottom, right;
} wm_rect_t;

typedef struct {
    uintptr_t address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
} fb_t;

typedef struct {
    wm_rect_t position;
    bool left_button;
    bool right_button;
} wm_click_event_t;

typedef struct {
    uint32_t keycode;
    bool pressed;
    char repr;
    // TODO include modifiers
} wm_kbd_event_t;

/* This structure is how a window can get its user input.
 * `type` is one of the `WM_EVENT_*` flags, and describe valid fields:
 *  - `WM_EVENT_{CLICK,MOUSE_MOVE}` -> `mouse` is valid,
 *  - `WM_EVENT_KBD` -> `kbd` is valid.
 */
typedef struct {
    uint32_t type;
    wm_click_event_t mouse;
    wm_kbd_event_t kbd;
} wm_event_t;

typedef struct {
    fb_t* fb;
    uint32_t flags;
} wm_param_open_t;

typedef struct {
    uint32_t win_id;
    wm_rect_t* clip;
} wm_param_render_t;

typedef struct {
    uint32_t win_id;
    wm_event_t* event;
} wm_param_event_t;

typedef struct {
    int64_t bg_color, // background color
    base_color, border_color, text_color, highlight, // title bar colors
    border_color2; // window border color
} color_scheme_t;