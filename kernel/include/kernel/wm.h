#pragma once

#include <kernel/fb.h>

#include <stdint.h>
#include <stdbool.h>
#include <list.h>

#include <kernel/uapi/uapi_wm.h>

#define WM_NOT_DRAWN  ((uint32_t) 1 << 31) // Window has _never_ been called wm_render_window

/* ufb: the window's buffer in userspace. Used by the client for drawing
 *  operations. We copy this buffer on request to `kfb`.
 * kfb: the drawn window's buffer held by the WM. This is used to redraw the
 *  window when we're not in the window's address space.
 */
typedef struct _wm_window_t {
    fb_t ufb;
    fb_t kfb;
    int32_t x;
    int32_t y;
    uint32_t id;
    uint32_t flags;
    wm_event_t event;
} wm_window_t;

// We exposed `wm_rect_t` to userspace, rename it here for convenience
typedef wm_rect_t rect_t;

void init_wm();

uint32_t wm_open_window(fb_t* fb, uint32_t flags);
void wm_close_window(uint32_t win_id);
void wm_render_window(uint32_t win_id, rect_t* clip);
void wm_get_event(uint32_t win_id, wm_event_t* event);

// rect-handling functions
rect_t* rect_new_copy(rect_t r);
list_t* rect_split_by(rect_t a, rect_t b);
rect_t rect_from_window(wm_window_t* win);
void rect_subtract_clip_rect(list_t* rects, rect_t clip);
void rect_add_clip_rect(list_t* rects, rect_t clip);
void print_rect(rect_t* r);
bool rect_intersect(rect_t a, rect_t b);
void rect_clear_clipped(list_t* rects);