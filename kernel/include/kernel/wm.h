#pragma once

#include <kernel/fb.h>
#include <kernel/mouse.h>
#include <kernel/kbd.h>

#include <stdint.h>
#include <stdbool.h>
#include <list.h>
#include <ringbuffer.h>

#include <kernel/uapi/uapi_wm.h>

#include <gui_meta.h>

#define WM_NOT_DRAWN  ((uint32_t) 1 << 31) // Window has _never_ been called wm_render_window

typedef struct {
    int32_t x, y, w, h;
} rect_t;

typedef struct {
    int32_t x, y;
} point_t;

/* ufb: the window's buffer in userspace. Used by the client for drawing
 *  operations. We copy this buffer on request to `kfb`.
 * kfb: the drawn window's buffer held by the WM. This is used to redraw the
 *  window when we're not in the window's address space.
 */
typedef struct _wm_window_t {
    fb_t ufb;
    fb_t kfb;
    point_t* pos;
    uint32_t id;
    uint32_t flags;
    ringbuffer_t* events;
    bool is_hovered;
} wm_window_t;

void init_wm();

uint32_t wm_open_window(fb_t* fb, uint32_t flags);
void wm_close_window(uint32_t win_id);
void wm_render_window(uint32_t win_id, wm_rect_t* clip);
void wm_get_event(uint32_t win_id, wm_event_t* event);

// rect-handling functions
wm_rect_t* rect_new_copy(wm_rect_t r);
list_t* rect_split_by(wm_rect_t a, wm_rect_t b);
wm_rect_t rect_from_window(wm_window_t* win);
void rect_subtract_clip_rect(list_t* rects, wm_rect_t clip);
void rect_add_clip_rect(list_t* rects, wm_rect_t clip);
void print_rect(wm_rect_t* r);
bool rect_intersect(wm_rect_t a, wm_rect_t b);
void rect_clear_clipped(list_t* rects);

void wm_draw_window(wm_window_t* win, wm_rect_t rect);
void wm_partial_draw_window(wm_window_t* win, wm_rect_t rect);
void wm_refresh_screen();
void wm_refresh_partial(wm_rect_t clip);
void wm_assign_position(wm_window_t* win);
void wm_assign_z_orders();
void wm_raise_window(wm_window_t* win);
list_t* wm_get_window(uint32_t id);
void wm_print_windows();
list_t* wm_get_windows_above(wm_window_t* win);
wm_rect_t wm_mouse_to_rect(mouse_t mouse);
void wm_draw_mouse(wm_rect_t new);
void wm_mouse_callback(mouse_t curr);
void wm_kbd_callback(kbd_event_t event);