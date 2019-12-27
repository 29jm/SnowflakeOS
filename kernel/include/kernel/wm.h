#ifndef COMP_H
#define COMP_H

#include <kernel/fb.h>

#include <stdint.h>
#include <stdbool.h>

#define WM_NORMAL     0
#define WM_BACKGROUND 1
#define WM_FOREGROUND 2
#define WM_NOT_DRAWN  4 // Window has _never_ been called wm_render_window

/* ufb: the window's buffer in userspace. Used by the client for drawing
 *  operations. We copy this buffer on request to `kfb`.
 * kfb: the drawn window's buffer held by the WM. This is used to redraw the
 *  window when we're not in the window's address space.
 */
typedef struct _wm_window_t {
	struct _wm_window_t* next;
	struct _wm_window_t* prev; // TODO: remove prev
	fb_t ufb;
	fb_t kfb;
	uint32_t x;
	uint32_t y;
	uint32_t z;
	uint32_t id;
	uint32_t flags;
} wm_window_t;

void init_wm();

uint32_t wm_open_window(fb_t* fb, uint32_t flags);
void wm_close_window(uint32_t win_id);
void wm_render_window(uint32_t win_id);

#endif