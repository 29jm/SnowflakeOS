#pragma once

#include <kernel/fb.h>

#include <stdint.h>
#include <stdbool.h>
#include <list.h>
#include <ringbuffer.h>

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
    point_t* pos;
    uint32_t id;
    uint32_t flags;
    bool being_dragged;
    ringbuffer_t* events;
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

/* Declared in here to later use in syscall
 */
list_t* wm_get_window(uint32_t id);