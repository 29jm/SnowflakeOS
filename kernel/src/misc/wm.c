#include <kernel/wm.h>
#include <kernel/list.h>
#include <kernel/sys.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
	uint32_t top, left, bottom, right;
} rect_t;

list_t* rect_split_by(rect_t* a, rect_t* b);
rect_t* rect_from_window(wm_window_t* win);
rect_t rect_from_window_val(wm_window_t* win);
void print_rect(rect_t* r);
void wm_refresh_screen();
void wm_assign_position(wm_window_t* win);
void wm_assign_z_orders();
wm_window_t* wm_get_window(uint32_t id);
void wm_print_windows();

static uint32_t id_count = 0;
static list_t* windows;
static list_t* clip_rects;

static fb_t screen_fb;
static fb_t fb;

void init_wm() {
	screen_fb = fb_get_info();
	fb = screen_fb;
	fb.address = (uintptr_t) kamalloc(screen_fb.height*screen_fb.pitch, 0x1000);

	windows = list_new();
	clip_rects = list_new();
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
		.flags = flags // TODO: mask flags
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

	// TODO: refresh only windows above this one
	wm_refresh_screen();
}

/* Helpers */

rect_t* rect_new(uint8_t top, uint32_t left, uint32_t bottom, uint32_t right) {
	rect_t* rect = (rect_t*) kmalloc(sizeof(rect_t));

	if (!rect) {
		return NULL;
	}


/* Clipping functions */

/* Returns whether two rectangular areas intersect.
 * Note that touching isn't intersecting.
 */
bool rect_intersect(rect_t* a, rect_t* b) {
	return a->left <= b->right && a->right >= b->left &&
	       a->top <= b->bottom && a->bottom >= b->top;
}

/* Allocates the specified `rect_t` on the stack.
 */
rect_t* rect_new(uint32_t t, uint32_t l, uint32_t b, uint32_t r) {
	rect_t* rect = (rect_t*) kmalloc(sizeof(rect_t));

	*rect = (rect_t) {
		.top = t, .left = l, .bottom = b, .right = r
	};

	return rect;
}

/* Pretty-prints a `rect_t`.
 */
void print_rect(rect_t* r) {
	printf("top:%d, left:%d, bottom:%d, right:%d\n",
		r->top, r->left, r->bottom, r->right);
}

/* Removes a rectangle `clip` from the union of `rects` by splitting intersecting
 * rects by `clip`.
 */
void rect_subtract_clip_rect(list_t* rects, rect_t* clip) {
	for (uint32_t i = 0; i < rects->count; i++) {
		rect_t* current = list_get_at(rects, i); // O(n²)

		if (rect_intersect(current, clip)) {
			list_t* splits = rect_split_by(list_remove_at(rects, i), clip);
			uint32_t n_splits = splits->count;

			while (splits->count) {
				list_add_front(rects, list_remove_at(splits, 0));
			}

			kfree(current);
			kfree(splits);

			// Skip the rects we inserted at the front and those already checked
			// Mind the end of loop increment
			i = n_splits + i - 1;
		}
	}
}

/* Add a clipping rectangle to the area spanned by `rects` by splitting
 * intersecting rects by `clip`.
 * Note: `clip` isn't copied, so it mustn't be freed right after this call.
 */
void rect_add_clip_rect(list_t* rects, rect_t* clip) {
	rect_subtract_clip_rect(rects, clip);
	list_add_front(rects, clip);
}

/* Empties the list while freeing its elements.
 */
void rect_clear_clipped(list_t* rects) {
	while (rects->count) {
		kfree(list_remove_at(rects, 0));
	}
}

/* Splits the original rectangle in more rectangles that cover the area
 *     `original \ split`
 * in set-theoric terms. Returns a list of those rectangles.
 */
list_t* rect_split_by(rect_t* original, rect_t* split) {
	list_t* list = list_new();

	if (!list) {
		return NULL;
	}

	rect_t rect = *original;
	rect_t* tmp;

	// Split by the left edge
	if (split->left > rect.left && split->left < rect.right) {
		tmp = rect_new(rect.top, rect.left, rect.bottom, split->left - 1);

		if (!tmp) {
			goto error;
		}

		list_add(list, tmp);
		rect.left = split->left;
	}

	// Split by the top edge
	if (split->top > rect.top && split->top < rect.bottom) {
		tmp = rect_new(rect.top, rect.left, split->top - 1, rect.right);

		if (!tmp) {
			goto error;
		}

		list_add(list, tmp);
		rect.top = split->top;
	}

	// Split by the right edge
	if (split->right > rect.left && split->right < rect.right) {
		tmp = rect_new(rect.top, split->right + 1, rect.bottom, rect.right);

		if (!tmp) {
			goto error;
		}

		list_add(list, tmp);
		rect.right = split->right;
	}

	if (split->bottom > rect.top && split->bottom < rect.bottom) {
		tmp = rect_new(split->bottom + 1, rect.left, rect.bottom, rect.right);

		if (!tmp) {
			goto error;
		}

		list_add(list, tmp);
		rect.bottom = split->bottom;
	}

	return list;

error:
	while (list->count) {
		kfree(list_remove_at(list, 0));
	}

	kfree(list);

	return NULL;
}

/* Returns a heap-allocated rectangle corresponding to the area spanned by the
 * window.
 */
rect_t* rect_from_window(wm_window_t* win) {
	return rect_new(win->y, win->x, win->y+win->kfb.height, win->x+win->kfb.width);
}

/* Returns a rectangle corresponding to the area spanned by the window.
 */
rect_t rect_from_window_val(wm_window_t* win) {
	return (rect_t) {
		.top = win->y,
		.left = win->x,
		.bottom = win->y+win->kfb.height,
		.right = win->x+win->kfb.width
	};
}

/* Returns a list of all windows above `win` that overlap with it.
 */
list_t* wm_get_windows_above(wm_window_t* win) {
	list_t* list = list_new();

	uint32_t idx = list_get_index_of(windows, win);
	rect_t* win_rect = rect_from_window(win);

	for (uint32_t i = idx + 1; i < windows->count; i++) {
		wm_window_t* next = list_get_at(windows, i);
		rect_t* rect = rect_from_window(next); // TODO: remove allocation

		if (rect_intersect(win_rect, rect)) {
			list_add_front(list, next);
		}

		kfree(rect);
	}

	kfree(win_rect);

	return list;
}

/* Draw a clipped window to the framebuffer. If the clipping rect is NULL,
 * draw the entire window.
 */
void wm_partial_draw_window(wm_window_t* win, rect_t* clip) {
	rect_t rect;

	if (!clip) {
		clip = &rect;

		clip->top = win->y;
		clip->left = win->x;
		clip->bottom = win->y + win->kfb.height;
		clip->right = win->x + win->kfb.width;
	}

	// Clamp the clipping rect to the window rect
	if (clip->top > win->y) {
		clip->top = win->y;
	}

	if (clip->left < win->x) {
		clip->left = win->x;
	}

	if (clip->bottom > win->y + win->kfb.height) {
		clip->bottom = win->y + win->kfb.height;
	}

	if (clip->right > win->x + win->kfb.width) {
		clip->right = win->x + win->kfb.width;
	}

	fb_t* wfb = &win->kfb;
	uintptr_t fb_off = fb.address + clip->top*fb.pitch + clip->left*fb.bpp/8;
	uintptr_t win_off = wfb->address + (clip->left - win->x)*wfb->bpp/8;
	uint32_t len = (clip->right - clip->left + 1)*wfb->bpp/8;

	for (uint32_t y = clip->top; y < clip->bottom; y++) {
		memcpy((void*) fb_off, (void*) (win_off + (y - win->y)*wfb->pitch), len);
		fb_off += fb.pitch;
	}
}

/* Draws the given window to the screen using the current clipping rects.
 */
void wm_draw_window(wm_window_t* win) {
	rect_t* win_rect = rect_from_window(win);
	list_t* clip_windows = wm_get_windows_above(win);

	rect_clear_clipped(clip_rects);
	rect_add_clip_rect(clip_rects, win_rect);

	while (clip_windows->count) {
		wm_window_t* cw = list_remove_at(clip_windows, 0);
		rect_t clip = rect_from_window_val(cw);
		rect_subtract_clip_rect(clip_rects, &clip);
	}

	// No need to free the elements: they're from `windows`, and we don't
	// free `win_rect`, it's one of the clipping rectangles.
	kfree(clip_windows);

	if (clip_rects->count) {
		for (uint32_t i = 0; i < clip_rects->count; i++) {
			rect_t* clip = list_get_at(clip_rects, i); // O(n²)
			wm_partial_draw_window(win, clip);
		}
	} else {
		wm_partial_draw_window(win, NULL);
	}
}

/* Redraws every visible area of the screen.
 */
void wm_refresh_screen() {
	for (uint32_t i = 0; i < windows->count; i++) {
		wm_window_t* win = list_get_at(windows, i); // O(n²)

		wm_draw_window(win);
	}

	// Update the screen
	fb_render(fb);
}

/* Other helpers */

	for (uint32_t i = 1; i < windows->count; i++) {
		wm_window_t* win = list_get_at(windows, i);

		if ((win->flags & flags) == flags) {
			return win;
		}
	}

	return NULL;
}

void wm_print_windows() {
	for (uint32_t i = 1; i < windows->count; i++) {
		wm_window_t* win = list_get_at(windows, i);
		printf("%d -> ", win->id);
	}

	printf("none\n");
}

wm_window_t* wm_get_window(uint32_t id) {
	if (!windows->count) {
		return NULL;
	}

	for (uint32_t i = 0; i < windows->count; i++) {
		wm_window_t* win = list_get_at(windows, i);

		if (win->id == id) {
			return win;
		}
	}

	return NULL;
}

/* TODO: revamp entirely.
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