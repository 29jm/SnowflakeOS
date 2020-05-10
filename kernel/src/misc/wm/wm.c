#include <kernel/wm.h>
#include <kernel/mouse.h>
#include <kernel/kbd.h>
#include <kernel/list.h>
#include <kernel/sys.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MOUSE_SIZE 3

void wm_draw_window(wm_window_t* win, rect_t rect);
void wm_partial_draw_window(wm_window_t* win, rect_t rect);
void wm_refresh_screen();
void wm_refresh_partial(rect_t clip);
void wm_assign_position(wm_window_t* win);
void wm_assign_z_orders();
void wm_raise_window(wm_window_t* win);
wm_window_t* wm_get_window(uint32_t id);
void wm_print_windows();
list_t* wm_get_windows_above(wm_window_t* win);
rect_t wm_mouse_to_rect(mouse_t mouse);
void wm_draw_mouse(rect_t old, rect_t new);
void wm_mouse_callback(mouse_t curr);
void wm_kbd_callback(kbd_event_t event);

/* Windows are ordered by z-index in this list, e.g. the foremost window is in
 * the last position.
 */
static list_t* windows;
static uint32_t id_count = 0;
static fb_t fb;
static mouse_t mouse;

void init_wm() {
	fb = fb_get_info();
	windows = list_new();

	mouse.x = fb.width/2;
	mouse.y = fb.height/2;

	mouse_set_callback(wm_mouse_callback);
	kbd_set_callback(wm_kbd_callback);
}

/* Associates a buffer with a window id. The calling program will then be able
 * to use this id to render the buffer through the window manager.
 */
uint32_t wm_open_window(fb_t* buff, uint32_t flags) {
	wm_window_t* win = (wm_window_t*) kmalloc(sizeof(wm_window_t));

	*win = (wm_window_t) {
		.ufb = *buff,
		.kfb = *buff,
		.id = id_count++,
		.flags = flags | WM_NOT_DRAWN
	};

	win->kfb.address = (uintptr_t) kmalloc(buff->height*buff->pitch);

	list_add(windows, win);
	wm_assign_position(win);
	wm_assign_z_orders();

	return win->id;
}

void wm_close_window(uint32_t win_id) {
	wm_window_t* win = wm_get_window(win_id);
	rect_t rect = rect_from_window(win);

	if (win) {
		uint32_t idx = list_get_index_of(windows, win);
		list_remove_at(windows, idx);
		kfree((void*) win->kfb.address);
		kfree((void*) win);
	} else {
		printf("[WM] Close: failed to find window of id %d\n", win_id);
	}

	wm_assign_z_orders();
	wm_refresh_partial(rect);
}

/* System call interface to draw a window. `clip` specifies which part to copy
 * from userspace and redraw. If `clip` is NULL, the whole window is redrawn.
 */
void wm_render_window(uint32_t win_id, rect_t* clip) {
	wm_window_t* win = wm_get_window(win_id);
	rect_t rect;

	if (!win) {
		printf("[WM] Render called by invalid window, id %d\n", win_id);
		return;
	}

	if (!clip) {
		clip = &rect;
		*clip = (rect_t) {
			.top = 0, .left = 0,
			.bottom = win->ufb.height - 1, .right = win->ufb.width - 1
		};
	}

	// Copy the window's buffer in the kernel
	uintptr_t off = clip->top*win->ufb.pitch + clip->left*win->ufb.bpp/8;
	uint32_t len = (clip->right - clip->left + 1)*win->ufb.bpp/8;

	for (int32_t i = clip->top; i <= clip->bottom; i++) {
		memcpy((void*) (win->kfb.address + off), (void*) (win->ufb.address + off), len);
		off += win->ufb.pitch;
	}

	rect = rect_from_window(win);
	wm_draw_window(win, *clip);

	if (win->flags & WM_NOT_DRAWN) {
		rect_t r = wm_mouse_to_rect(mouse);
		win->flags &= ~WM_NOT_DRAWN;

		wm_draw_mouse(r, r);
	}
}

void wm_get_event(uint32_t win_id, wm_event_t* event) {
	wm_window_t* win = wm_get_window(win_id);

	if (!win) {
		printf("[WM] Invalid window %d\n", win_id);
		return;
	}

	*event = win->event;
	memset(&win->event, 0, sizeof(wm_event_t));
}

/* Window management stuff */

/* Puts `win` at the highest z-level.
 */
void wm_raise_window(wm_window_t* win) {
	uint32_t idx = list_get_index_of(windows, win);
	list_add(windows, list_remove_at(windows, idx));

	wm_assign_z_orders();

	rect_t rect = rect_from_window(win);
	wm_draw_window(win, rect);
}

/* Sets the position of the window according to obscure rules.
 * TODO: revamp entirely.
 */
void wm_assign_position(wm_window_t* win) {
	if (win->kfb.width == fb.width) {
		win->x = 0;
		win->y = 0;

		return;
	}

	int max_x = fb.width - win->ufb.width;
	int max_y = fb.height - win->ufb.height;

	win->x = rand() % max_x;
	win->y = rand() % max_y;
}

/* Makes sure that z-level related flags are respected.
 */
void wm_assign_z_orders() {
	for (uint32_t i = 0; i < windows->count; i++) {
		wm_window_t* win = list_get_at(windows, i); // O(n²)

		if (win->flags & WM_BACKGROUND && i != 0) {
			list_add_front(windows, list_remove_at(windows, i));
		} else if (win->flags & WM_FOREGROUND) {
			list_add(windows, list_remove_at(windows, i));
		}
	}
}

/* Clipping stuff */

/* Draw the given part of the window to the framebuffer.
 * Note that the given clip is allowed to be partially outside the window.
 */
void wm_partial_draw_window(wm_window_t* win, rect_t clip) {
	fb_t* wfb = &win->kfb;
	rect_t win_rect = rect_from_window(win);

	// Clamp the clipping rect to the window rect
	if (clip.top < win_rect.top) {
		clip.top = win_rect.top;
	}

	if (clip.left < win_rect.left) {
		clip.left = win_rect.left;
	}

	if (clip.bottom > win_rect.bottom) {
		clip.bottom = win_rect.bottom;
	}

	if (clip.right > win_rect.right) {
		clip.right = win_rect.right;
	}

	// Compute offsets; remember that `right` and `bottom` are inclusive
	uintptr_t fb_off = fb.address + clip.top*fb.pitch + clip.left*fb.bpp/8;
	uintptr_t win_off = wfb->address + (clip.left - win->x)*wfb->bpp/8;
	uint32_t len = (clip.right - clip.left + 1)*wfb->bpp/8;

	for (int32_t y = clip.top; y <= clip.bottom; y++) {
		memcpy((void*) fb_off, (void*) (win_off + (y - win->y)*wfb->pitch), len);
		fb_off += fb.pitch;
	}
}

/* Draws the visible parts of the window that are within the given clipping
 * rect.
 */
void wm_draw_window(wm_window_t* win, rect_t rect) {
	rect_t win_rect = rect_from_window(win);
	list_t* clip_windows = wm_get_windows_above(win);
	list_t* clip_rects = list_new();

	rect_add_clip_rect(clip_rects, rect);

	// Convert covering windows to clipping rects
	while (clip_windows->count) {
		wm_window_t* cw = list_remove_at(clip_windows, 0);
		rect_t clip = rect_from_window(cw);
		rect_subtract_clip_rect(clip_rects, clip);
	}

	for (uint32_t i = 0; i < clip_rects->count; i++) {
		rect_t* clip = list_get_at(clip_rects, i); // O(n²)

		if (!rect_intersect(*clip, win_rect)) {
			continue;
		}

		wm_partial_draw_window(win, *clip);
	}

	rect_clear_clipped(clip_rects);
	kfree(clip_rects);
	kfree(clip_windows);
}

/* Refreshes only a part of the screen.
 * TODO: allow refreshing empty space, filled with black.
 */
void wm_refresh_partial(rect_t clip) {
	for (uint32_t i = 0; i < windows->count; i++) {
		wm_window_t* win = list_get_at(windows, i); // O(n²)
		rect_t rect = rect_from_window(win);

		if (rect_intersect(clip, rect)) {
			wm_draw_window(win, clip);
		}
	}
}

/* Redraws every visible area of the screen.
 */
void wm_refresh_screen() {
	rect_t screen_rect = {
		.top = 0, .left = 0, .bottom = fb.height, .right = fb.width
	};

	wm_refresh_partial(screen_rect);
}

/* Other helpers */

void wm_print_windows() {
	for (uint32_t i = 1; i < windows->count; i++) {
		wm_window_t* win = list_get_at(windows, i); // O(n²)
		printf("%d -> ", win->id);
	}

	printf("none\n");
}

/* Returns a list of all windows above `win` that overlap with it.
 */
list_t* wm_get_windows_above(wm_window_t* win) {
	list_t* list = list_new();

	uint32_t idx = list_get_index_of(windows, win);
	rect_t win_rect = rect_from_window(win);

	for (uint32_t i = idx + 1; i < windows->count; i++) {
		wm_window_t* next = list_get_at(windows, i);
		rect_t rect = rect_from_window(next);

		if (rect_intersect(win_rect, rect)) {
			list_add_front(list, next);
		}
	}

	return list;
}

/* Return the window object corresponding to the given id, NULL if none match.
 */
wm_window_t* wm_get_window(uint32_t id) {
	for (uint32_t i = 0; i < windows->count; i++) {
		wm_window_t* win = list_get_at(windows, i);

		if (win->id == id) {
			return win;
		}
	}

	return NULL;
}

/* Mouse handling functions */

rect_t wm_mouse_to_rect(mouse_t mouse) {
	return (rect_t) {
		.top = mouse.y, .left = mouse.x,
		.bottom = mouse.y+MOUSE_SIZE, .right = mouse.x+MOUSE_SIZE
	};
}

/* Returns the foremost window containing the point at (x, y), NULL if none match.
 */
wm_window_t* wm_window_at(int32_t x, int32_t y) {
	for (int32_t i = windows->count - 1; i >= 0; i--) {
		wm_window_t* win = list_get_at(windows, i);
		rect_t r = rect_from_window(win);

		if (y >= r.top && y <= r.bottom && x >= r.left && x <= r.right) {
			return win;
		}
	}

	return NULL;
}

void wm_draw_mouse(rect_t old, rect_t new) {
	wm_refresh_partial(old);

	uintptr_t addr = fb.address + new.top*fb.pitch + new.left*fb.bpp/8;

	for (int32_t y = 0; y < new.bottom - new.top; y++) {
		memset((void*) addr, 0, (new.right - new.left)*fb.bpp/8);
		addr += fb.pitch;
	}
}

/* Handles mouse events.
 */
void wm_mouse_callback(mouse_t raw_curr) {
	static mouse_t raw_prev;
	static wm_window_t* dragged = NULL;
	static bool been_dragged = false;

	const mouse_t prev = mouse;
	const float sens = 0.7f;
	const int32_t max_x = fb.width - MOUSE_SIZE;
	const int32_t max_y = fb.height - MOUSE_SIZE;

	// Move the cursor
	int32_t dx = (raw_curr.x - raw_prev.x)*sens;
	int32_t dy = (raw_curr.y - raw_prev.y)*sens;

	mouse.x += dx;
	mouse.y += dy;

	mouse.x = mouse.x > max_x ? max_x : mouse.x;
	mouse.y = mouse.y > max_y ? max_y : mouse.y;
	mouse.x = mouse.x < 0 ? 0 : mouse.x;
	mouse.y = mouse.y < 0 ? 0 : mouse.y;

	// Raises and drag starts
	if (!raw_prev.left_pressed && raw_curr.left_pressed) {
		wm_window_t* clicked = wm_window_at(mouse.x, mouse.y);

		if (clicked && !(clicked->flags & WM_BACKGROUND)) {
			wm_raise_window(clicked);

			dragged = clicked;
		}
	}

	// Drags
	if (raw_prev.left_pressed && raw_curr.left_pressed) {
		if (dragged && (dx || dy)) {
			rect_t rect = rect_from_window(dragged);

			if (!(rect.left + dx < 0 || rect.right + dx > (int32_t) fb.width ||
			    	rect.top + dy < 0 || rect.bottom + dy > (int32_t) fb.height)) {
				been_dragged = true;

				dragged->x += dx;
				dragged->y += dy;

				rect_t new_rect = rect_from_window(dragged);

				wm_refresh_partial(rect);
				wm_draw_window(dragged, new_rect);

				rect = wm_mouse_to_rect(mouse);
				wm_draw_mouse(rect, rect);
			}
		}
	}

	// Clicks and drag ends
	if (raw_prev.left_pressed && !raw_curr.left_pressed) {
		if (!been_dragged) {
			rect_t r = rect_from_window(dragged);

			dragged->event.type |= WM_EVENT_CLICK;
			dragged->event.mouse.position = wm_mouse_to_rect(mouse);
			dragged->event.mouse.position.top -= r.top;
			dragged->event.mouse.position.left -= r.left;
		}

		been_dragged = false;
		dragged = NULL;
	}

	// Redraw the mouse if needed
	if (dx || dy || dragged) {
		rect_t prev_pos = wm_mouse_to_rect(prev);
		rect_t curr_pos = wm_mouse_to_rect(mouse);

		wm_draw_mouse(prev_pos, curr_pos);
	}

	// Update the saved state
	raw_prev = raw_curr;
}

void wm_kbd_callback(kbd_event_t event) {
	if (windows->count) {
		wm_window_t* win = list_get_at(windows, windows->count - 1);

		win->event.type |= WM_EVENT_KBD;
		win->event.kbd.keycode = event.keycode;
		win->event.kbd.pressed = event.pressed;
		win->event.kbd.repr = event.repr;
	}
}