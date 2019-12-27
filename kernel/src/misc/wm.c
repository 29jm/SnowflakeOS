#include <kernel/wm.h>
#include <kernel/mem.h>
#include <kernel/sys.h>

#include <stdio.h>
#include <string.h>

uint32_t wm_get_best_x(wm_window_t* win);
uint32_t wm_get_max_z();
void wm_append_window(wm_window_t* win);
void wm_assign_z_orders();
void wm_remove_window(uint32_t id);
wm_window_t* wm_get_window(uint32_t id);
wm_window_t* wm_find_with_z(uint32_t id);

static uint32_t id_count = 0;
static wm_window_t* windows;
static fb_t screen_fb;
static fb_t fb;

void init_wm() {
	screen_fb = fb_get_info();
	fb = screen_fb;
	fb.address = (uintptr_t) kamalloc(screen_fb.height*screen_fb.pitch, 0x1000);
}

/* Associates a buffer with a window id. The calling program will then be able
 * to use this id to render the buffer through the window manager.
 */
uint32_t wm_open_window(fb_t* buff, uint32_t flags) {
	wm_window_t* win = (wm_window_t*) kmalloc(sizeof(wm_window_t));

	*win = (wm_window_t) {
		.prev = NULL,
		.next = NULL,
		.ufb = *buff,
		.kfb = *buff,
		.y = screen_fb.height/2-buff->height/2,
		.id = id_count++,
		.flags = WM_NOT_DRAWN | flags
	};

	win->kfb.address = (uintptr_t) kamalloc(buff->height*buff->pitch, 0x10);

	win->x = wm_get_best_x(win);
	win->z = windows ? wm_get_max_z() + 1 : 0;

	wm_append_window(win);
	wm_assign_z_orders();

	return win->id;
}

void wm_close_window(uint32_t win_id) {
	printf("[WM] Closing window %d\n", win_id);

	wm_remove_window(win_id);
	wm_assign_z_orders();
}

/* If the current z-order being drawn is the z-order of the given window, copy
 * it to the off-screen buffer. Otherwise, do nothing.
 * If the window was the topmost window (highest z-order), then the off-screen
 * buffer is flushed to the screen.
 */
void wm_render_window(uint32_t win_id) {
	wm_window_t* win = wm_get_window(win_id);

	if (!win) {
		printf("[WM] Render called by invalid window, id %d\n", win_id);
		return;
	}

	// Copy the window's buffer in the kernel
	// TODO: realloc kfb on size change
	uint32_t win_size = win->ufb.height*win->ufb.pitch;
	memcpy((void*) win->kfb.address, (void*) win->ufb.address, win_size);

	// Make sure the window is marked as drawn
	win->flags &= ~WM_NOT_DRAWN;

	// Redraw all windows in z-order
	uint32_t max_z = wm_get_max_z();

	for (uint32_t z = 0; z <= max_z; z++) {
		wm_window_t* win = wm_find_with_z(z);

		// Skip windows which have been opened but haven't been drawn yet
		if (win->flags & WM_NOT_DRAWN) {
			continue;
		}

		uint32_t* off = (uint32_t*) (fb.address + win->y*fb.pitch + win->x*fb.bpp/8);

		for (uint32_t i = 0; i < win->kfb.height; i++) {
			memcpy(off, (void*) (win->kfb.address + i*win->kfb.pitch), win->kfb.pitch);
			off = (uint32_t*) ((uintptr_t) off + fb.pitch);
		}
	}

	// Update the screen
	fb_render(fb);
}

/* Helpers */

void wm_append_window(wm_window_t* win) {
	if (!windows) {
		windows = win;
		win->prev = NULL;
		return;
	}

	wm_window_t* current = windows;

	while (current->next) {
		current = current->next;
	}

	win->prev = current;
	current->next = win;
}

void wm_remove_window(uint32_t id) {
	wm_window_t* current = windows;

	while (current->next) {
		if (current->id == id) {
			if (current->prev) {
				current->prev->next = current->next;
			} else {
				windows = current->next;
			}

			return;
		}

		current = current->next;
	}
}

wm_window_t* wm_get_window(uint32_t id) {
	wm_window_t* current = windows;

	if (!current) {
		return NULL;
	}

	do {
		if (current->id == id) {
			return current;
		}
	} while ((current = current->next));

	return NULL;
}

uint32_t wm_count_windows() {
	wm_window_t* current = windows;
	uint32_t count = 1;

	if (!windows) {
		return 0;
	}

	while (current->next) {
		current = current->next;
		count++;
	}

	return count;
}

/* TODO: revamp entirely.
 */
uint32_t wm_get_best_x(wm_window_t* win) {
	wm_window_t* current = windows;

	if (!windows) {
		return 0;
	}

	if (win->kfb.width == fb.width) {
		return 0;
	}

	while (current->next) {
		current = current->next;
	}

	return current->x + 50;
}

uint32_t wm_get_max_z() {
	wm_window_t* current = windows;

	if (!windows) {
		return 0;
	}

	uint32_t m = current->z;

	while (current->next) {
		current = current->next;

		if (current->z > m) {
			m = current->z;
		}
	}

	return m;
}

wm_window_t* wm_find_with_flags(uint32_t flags) {
	wm_window_t* current = windows;

	if (!current) {
		return NULL;
	}

	do {
		if ((current->flags & flags) == flags) {
			return current;
		}
	} while ((current = current->next));

	return NULL;
}

wm_window_t* wm_find_with_z(uint32_t z) {
	wm_window_t* current = windows;

	if (!current) {
		return NULL;
	}

	do {
		if (current->z == z) {
			return current;
		}
	} while ((current = current->next));

	return NULL;
}

/* Makes sure there are no gaps in z orders and that flags related to z orders
 * are respected.
 * TODO: when accounting for flags, shift z orders instead of swapping.
 */
void wm_assign_z_orders() {
	for (uint32_t z = 0; z < wm_count_windows(); z++) {
		wm_window_t* win_z = wm_find_with_z(z);

		if (!win_z) {
			// Find the next present z
			uint32_t next_z = z+1;

			while (!wm_find_with_z(next_z)) {
				next_z++;
			}

			// Shift later z orders to fill the gap
			for (uint32_t zp = next_z; zp <= wm_get_max_z(); zp++) {
				wm_window_t* win = wm_find_with_z(zp);

				if (win) {
					win->z = z + zp - next_z;
				}
			}
		}
	}

	wm_window_t* bg = wm_find_with_flags(WM_BACKGROUND);

	if (bg && bg->z != 0) {
		wm_window_t* win = wm_find_with_z(0);
		win->z = bg->z;
		bg->z = 0;
	}

	wm_window_t* fg = wm_find_with_flags(WM_FOREGROUND);
	uint32_t max_z = wm_get_max_z();

	if (fg && fg->z != max_z) {
		wm_window_t* win = wm_find_with_z(max_z);
		win->z = fg->z;
		fg->z = max_z;
	}
}