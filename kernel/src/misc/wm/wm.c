#include <kernel/wm.h>
#include <kernel/mouse.h>
#include <kernel/list.h>
#include <kernel/sys.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

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

/* Windows are ordered by z-index in this list, e.g. the foremost window is in
 * the last position.
 */
static list_t* windows;
static uint32_t id_count = 0;

static fb_t screen_fb;
static fb_t fb;

void init_wm() {
	screen_fb = fb_get_info();
	fb = screen_fb;
	fb.address = (uintptr_t) kamalloc(screen_fb.height*screen_fb.pitch, 0x1000);
	windows = list_new();
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
		.flags = flags
	};

	win->kfb.address = (uintptr_t) kmalloc(buff->height*buff->pitch);

	list_add(windows, win);
	wm_assign_position(win);
	wm_assign_z_orders();

	return win->id;
}

void wm_close_window(uint32_t win_id) {
	wm_window_t* win = wm_get_window(win_id);

	if (win) {
		uint32_t idx = list_get_index_of(windows, win);
		list_remove_at(windows, idx);
		kfree((void*) win->kfb.address);
		kfree((void*) win);
	} else {
		printf("[WM] Close: failed to find window of id %d\n", win_id);
	}

	wm_assign_z_orders();
	wm_refresh_screen();
}

void wm_render_window(uint32_t win_id) {
	wm_window_t* win = wm_get_window(win_id);

	if (!win) {
		printf("[WM] Render called by invalid window, id %d\n", win_id);
		return;
	}

	// Copy the window's buffer in the kernel
	uint32_t win_size = win->ufb.height*win->ufb.pitch;
	memcpy((void*) win->kfb.address, (void*) win->ufb.address, win_size);

	wm_refresh_partial(rect_new_from_window(win));
}

/* Window management stuff */

/* Puts `win` at the highest z-level.
 */
void wm_raise_window(wm_window_t* win) {
	uint32_t idx = list_get_index_of(windows, win);

	list_add(windows, list_remove_at(windows, idx));
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
 */
void wm_partial_draw_window(wm_window_t* win, rect_t clip) {
	fb_t* wfb = &win->kfb;
	rect_t win_rect = rect_new_from_window(win);

	// Clamp the clipping rect to the window rect
	if (clip.top < win->y) {
		clip.top = win_rect.top;
	}

	if (clip.left < win->x) {
		clip.left = win_rect.left;
	}

	if (clip.bottom > win->y + win->kfb.height) {
		clip.bottom = win_rect.bottom;
	}

	if (clip.right > win->x + win->kfb.width) {
		clip.right = win_rect.right;
	}

	// Compute offsets; remember that `right` and `bottom` are inclusive
	uintptr_t fb_off = fb.address + clip.top*fb.pitch + clip.left*fb.bpp/8;
	uintptr_t win_off = wfb->address + (clip.left - win->x)*wfb->bpp/8;
	uint32_t len = (clip.right - clip.left + 1)*wfb->bpp/8;

	for (uint32_t y = clip.top; y <= clip.bottom; y++) {
		memcpy((void*) fb_off, (void*) (win_off + (y - win->y)*wfb->pitch), len);
		fb_off += fb.pitch;
	}
}

/* Draws the visible parts of the window that are within the given clipping
 * rect.
 */
void wm_draw_window(wm_window_t* win, rect_t rect) {
	rect_t win_rect = rect_new_from_window(win);

	if (!rect_intersect(&win_rect, &rect)) {
		return;
	}

	list_t* clip_windows = wm_get_windows_above(win);
	list_t* clip_rects = list_new();

	rect_add_clip_rect(clip_rects, rect);

	// Convert covering windows to clipping rects
	while (clip_windows->count) {
		wm_window_t* cw = list_remove_at(clip_windows, 0);
		rect_t clip = rect_new_from_window(cw);
		rect_subtract_clip_rect(clip_rects, clip);
	}

	for (uint32_t i = 0; i < clip_rects->count; i++) {
		rect_t* clip = list_get_at(clip_rects, i); // O(n²)

		wm_partial_draw_window(win, *clip);
	}

	rect_clear_clipped(clip_rects);
	kfree(clip_rects);
	kfree(clip_windows);
}

/* Refresh only a part of the screen.
 * Note: for simplicity, this takes only a clipping rect and not a list of
 *     them, though it would make sense.
 */
void wm_refresh_partial(rect_t clip) {
	for (uint32_t i = 0; i < windows->count; i++) {
		wm_window_t* win = list_get_at(windows, i); // O(n²)
		rect_t rect = rect_new_from_window(win);

		if (rect_intersect(&clip, &rect)) {
			wm_draw_window(win, clip);
		}
	}

	fb_render(fb);
}

/* Redraws every visible area of the screen.
 */
void wm_refresh_screen() {
	rect_t screen_rect = {
		// TODO: missing a +1 here?
		.top = 0, .left = 0, .bottom = screen_fb.height, .right = screen_fb.width
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
	rect_t win_rect = rect_new_from_window(win);

	for (uint32_t i = idx + 1; i < windows->count; i++) {
		wm_window_t* next = list_get_at(windows, i);
		rect_t rect = rect_new_from_window(next);

		if (rect_intersect(&win_rect, &rect)) {
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