#include <kernel/wm.h>
#include <kernel/mouse.h>
#include <kernel/kbd.h>
#include <kernel/sys.h>

#include <kernel/fs.h>

#include <stdio.h>
#include <string.h>
#include <list.h>
#include <stdlib.h>

#define MOUSE_SIZE 16
#define WM_EVENT_QUEUE_SIZE 5

void wm_draw_window(wm_window_t* win, rect_t rect);
void wm_partial_draw_window(wm_window_t* win, rect_t rect);
void wm_refresh_screen();
void wm_refresh_partial(rect_t clip);
void wm_assign_position(wm_window_t* win);
void wm_assign_z_orders();
void wm_raise_window(wm_window_t* win);
list_t* wm_get_window(uint32_t id);
void wm_print_windows();
list_t* wm_get_windows_above(wm_window_t* win);
rect_t wm_mouse_to_rect(mouse_t mouse);
void wm_draw_mouse(rect_t new);
void wm_mouse_callback(mouse_t curr);
void wm_kbd_callback(kbd_event_t event);

/* Windows are ordered by z-index in this list, e.g. the foremost window is in
 * the last position.
 */
static list_t windows;
static wm_window_t* focused;
static uint32_t id_count = 0;
static fb_t fb;
static mouse_t mouse;

void init_wm() {
    fb = fb_get_info();
    windows = LIST_HEAD_INIT(windows);

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
        .id = ++id_count,
        .flags = flags | WM_NOT_DRAWN,
        .events = ringbuffer_new(WM_EVENT_QUEUE_SIZE * sizeof(wm_event_t))
    };

    win->kfb.address = (uintptr_t) kmalloc(buff->height*buff->pitch);

    list_add_front(&windows, win);
    wm_assign_position(win);
    wm_assign_z_orders();
    wm_raise_window(win);

    return win->id;
}

void wm_close_window(uint32_t win_id) {
    list_t* item = wm_get_window(win_id);

    if (item) {
        wm_window_t* win = list_entry(item, wm_window_t);
        rect_t rect = rect_from_window(win);

        list_del(item);
        ringbuffer_free(win->events);
        kfree((void*) win->kfb.address);
        kfree((void*) win);

        if (!list_empty(&windows)) {
            wm_raise_window(list_last_entry(&windows, wm_window_t));
        }

        wm_refresh_partial(rect);
    } else {
        printke("close: failed to find window of id %d", win_id);
    }
}

/* System call interface to draw a window. `clip` specifies which part to copy
 * from userspace and redraw. If `clip` is NULL, the whole window is redrawn.
 */
void wm_render_window(uint32_t win_id, rect_t* clip) {
    list_t* item = wm_get_window(win_id);
    rect_t rect;

    if (!item) {
        printke("render called by invalid window, id %d", win_id);
        return;
    }

    wm_window_t* win = list_entry(item, wm_window_t);

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

    // Draw the window for real
    rect = rect_from_window(win);
    wm_draw_window(win, *clip);

    // Mark as drawn once
    if (win->flags & WM_NOT_DRAWN) {
        win->flags &= ~WM_NOT_DRAWN;
    }
}

void wm_get_event(uint32_t win_id, wm_event_t* event) {
    list_t* item = wm_get_window(win_id);

    if (!item) {
        printke("Get_event: invalid window %d", win_id);
        return;
    }

    wm_window_t* win = list_entry(item, wm_window_t);

    if (ringbuffer_available(win->events)) {
        ringbuffer_read(win->events, sizeof(wm_event_t), (uint8_t*)event);
    } else {
        memset(event, 0, sizeof(wm_event_t));
    }
}

/* Window management stuff */

/* Puts a window to the front, giving it focus.
 * In some case, this may not be possible, for instance if a modal window
 * has the focus, or if the given window is marked as a background window.
 * By design, _only_ this function affects focus.
 */
void wm_raise_window(wm_window_t* win) {
    list_t* win_iter;
    wm_window_t* win_bis;
    wm_event_t event;

    // Find the window's list_t* to maybe move it around
    list_for_each(win_iter, win_bis, &windows) {
        if (win_bis == win) {
            break;
        }
    }

    // This is the first window to be opened
    if (!focused) {
        focused = win;
        event.type = WM_EVENT_GAINED_FOCUS;
        ringbuffer_write(win->events, sizeof(wm_event_t), (uint8_t*)&event);
        return;
    }

    // Nothing to do
    if (win->flags & WM_BACKGROUND || focused == win) {
        return;
    }

    // Change focus only then
    event.type = WM_EVENT_LOST_FOCUS;
    ringbuffer_write(focused->events, sizeof(wm_event_t), (uint8_t*)&event);

    event.type = WM_EVENT_GAINED_FOCUS;
    ringbuffer_write(win->events, sizeof(wm_event_t), (uint8_t*)&event);
    focused = win;

    list_t* topmost;
    wm_window_t* w;

    /* Find the top most non-foreground window; we'll move the raised window
     * after it */
    list_for_each_entry_rev(topmost, w, &windows) {
        if (!(w->flags & WM_FOREGROUND)) {
            break;
        }
    }

    list_move(win_iter, topmost);

    // Redraw if possible. Not sure this is this function's responsibility.
    if (!(win->flags & WM_NOT_DRAWN)) {
        wm_draw_window(win, rect_from_window(win));
    }
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
    list_t* iter;
    wm_window_t* win;

    list_for_each(iter, win, &windows) {
        if (win->flags & WM_BACKGROUND) {
            list_del(iter);
            list_add_front(&windows, win);
            break;
        }
    }

    list_for_each(iter, win, &windows) {
        if (win->flags & WM_FOREGROUND) {
            list_del(iter);
            list_add(&windows, win);
            break;
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
    list_t clip_rects = LIST_HEAD_INIT(clip_rects);

    rect_add_clip_rect(&clip_rects, rect);

    // Convert covering windows to clipping rects
    while (!list_empty(clip_windows)) {
        wm_window_t* cw = list_first_entry(clip_windows, wm_window_t);
        list_del(list_first(clip_windows));
        rect_t clip = rect_from_window(cw);
        rect_subtract_clip_rect(&clip_rects, clip);
    }

    kfree(clip_windows);

    // Draw what's left
    rect_t* clip;
    list_for_each_entry(clip, &clip_rects) {
        if (rect_intersect(*clip, win_rect)) {
            wm_partial_draw_window(win, *clip);
        }
    }

    // Redraw the mouse
    rect_t mouse_rect = wm_mouse_to_rect(mouse);
    wm_draw_mouse(mouse_rect);

    rect_clear_clipped(&clip_rects);
}

/* Refreshes only a part of the screen.
 * TODO: allow refreshing empty space, filled with black.
 */
void wm_refresh_partial(rect_t clip) {
    list_t to_refresh = LIST_HEAD_INIT(to_refresh);
    list_add(&to_refresh, rect_new_copy(clip));

    wm_window_t* win;
    list_for_each_entry(win, &windows) {
        rect_t rect = rect_from_window(win);

        if (rect_intersect(rect, clip)) {
            wm_draw_window(win, clip);
            rect_subtract_clip_rect(&to_refresh, rect);
        }
    }

    // Draw black areas where a refresh was needed but no window was present
    rect_t* r;
    list_for_each_entry(r, &to_refresh) {
        uintptr_t off = fb.address + r->top*fb.pitch + r->left*fb.bpp/8;
        uint32_t size = (r->right - r->left + 1)*fb.bpp/8;

        for (int32_t j = r->top; j <= r->bottom; j++) {
            memset((void*) off, 0, size);
            off += fb.pitch;
        }
    }

    rect_clear_clipped(&to_refresh);
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
    printk("printing windows:");

    wm_window_t* win;
    list_for_each_entry(win, &windows) {
        printf("%d -> ", win->id);
    }

    printf("none\n");
}

/* Returns a list of all windows above `win` that overlap with it.
 */
list_t* wm_get_windows_above(wm_window_t* win) {
    rect_t win_rect = rect_from_window(win);
    list_t* list = kmalloc(sizeof(list_t));
    list_t* iter;
    wm_window_t* w;
    bool after = false;
    *list = LIST_HEAD_INIT(*list);

    list_for_each(iter, w, &windows) {
        if (after) {
            rect_t rect = rect_from_window(w);

            if (rect_intersect(win_rect, rect)) {
                list_add_front(list, w);
            }
        }

        if (w == win) {
            after = true;
        }
    }

    return list;
}

/* Return the window object corresponding to the given id, NULL if none match.
 */
list_t* wm_get_window(uint32_t id) {
    list_t* iter;
    wm_window_t* win;

    list_for_each(iter, win, &windows) {
        if (win->id == id) {
            return iter;
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
    list_t* iter;
    wm_window_t* win;

    list_for_each_entry_rev(iter, win, &windows) {
        rect_t r = rect_from_window(win);

        if (y >= r.top && y <= r.bottom && x >= r.left && x <= r.right) {
            return win;
        }
    }

    return NULL;
}

void wm_draw_mouse(rect_t new) {
    uintptr_t addr = fb.address + new.top*fb.pitch + new.left*fb.bpp/8;

    for (int32_t y = 0; y < new.bottom - new.top - 6; y++) {
        memset((void*) addr, 127, (y + 1) * fb.bpp/8);
        addr += fb.pitch;
    }

    addr += 4*fb.bpp/8;

    for (int32_t y = 0; y < 6; y++) {
        memset((void*) addr, 127, 3 * fb.bpp/8);
        addr += fb.bpp/8;
        addr += fb.pitch;
    }
}

/* Handles mouse events. This includes moving the cursor, moving windows along
 * with it, and distributing clicks.
 */
void wm_mouse_callback(mouse_t raw_curr) {
    static mouse_t raw_prev;
    static wm_window_t* dragged = NULL;
    static bool been_dragged = false;

    const mouse_t prev = mouse;
    const float sens = 0.7f;
    const int32_t max_x = fb.width - MOUSE_SIZE - 1;
    const int32_t max_y = fb.height - MOUSE_SIZE - 1;

    // Move the cursor
    float dx = (raw_curr.x - raw_prev.x)*sens;
    float dy = (raw_curr.y - raw_prev.y)*sens;

    mouse.x += dx;
    mouse.y += dy;

    mouse.x = mouse.x > max_x ? max_x : mouse.x;
    mouse.y = mouse.y > max_y ? max_y : mouse.y;
    mouse.x = mouse.x < 0 ? 0 : mouse.x;
    mouse.y = mouse.y < 0 ? 0 : mouse.y;

    // Raises and drag starts
    if (!raw_prev.left_pressed && raw_curr.left_pressed) {
        wm_window_t* clicked = wm_window_at(mouse.x, mouse.y);

        if (clicked) {
            wm_raise_window(clicked);
            dragged = clicked;
        }
    }

    // Drags
    if (raw_prev.left_pressed && raw_curr.left_pressed) {
        if (dragged && (dx || dy)) {
            rect_t rect = rect_from_window(dragged);

            if (!(rect.left + dx < 0 || rect.right + dx >= (int32_t) fb.width ||
                    rect.top + dy < 0 || rect.bottom + dy >= (int32_t) fb.height)) {
                been_dragged = true;

                dragged->x += dx;
                dragged->y += dy;

                rect_t new_rect = rect_from_window(dragged);

                wm_refresh_partial(rect);
                wm_draw_window(dragged, new_rect);

                rect = wm_mouse_to_rect(mouse);
                wm_draw_mouse(rect);
            }
        }
    }

    // Clicks and drag ends
    if (raw_prev.left_pressed && !raw_curr.left_pressed) {
        wm_event_t event;

        if (!been_dragged) {
            rect_t r = rect_from_window(dragged);

            event.type = WM_EVENT_CLICK;
            event.mouse.position = wm_mouse_to_rect(mouse);
            event.mouse.position.top -= r.top;
            event.mouse.position.left -= r.left;

            ringbuffer_write(dragged->events, sizeof(wm_event_t), (uint8_t*) &event);
        }

        been_dragged = false;
        dragged = NULL;
    }

    // Simple moves
    if (!raw_prev.left_pressed && !raw_curr.left_pressed) {
        wm_window_t* under_cursor = wm_window_at(mouse.x, mouse.y);
        wm_event_t event;

        if (under_cursor) {
            rect_t r = rect_from_window(under_cursor);

            event.type = WM_EVENT_MOUSE_MOVE;
            event.mouse.position = wm_mouse_to_rect(mouse);
            event.mouse.position.top -= r.top;
            event.mouse.position.left -= r.left;

            ringbuffer_write(under_cursor->events, sizeof(wm_event_t), (uint8_t*) &event);
        }
    }

    // Redraw the mouse if needed
    if (dx || dy || dragged) {
        rect_t prev_pos = wm_mouse_to_rect(prev);
        rect_t curr_pos = wm_mouse_to_rect(mouse);

        wm_refresh_partial(prev_pos);
        wm_draw_mouse(curr_pos);
    }

    // Update the saved state
    raw_prev = raw_curr;
}

void wm_kbd_callback(kbd_event_t event) {
    wm_event_t kbd_event;

    if (!list_empty(&windows)) {
        list_t* iter;
        wm_window_t* win;

        list_for_each_entry_rev(iter, win, &windows) {
            kbd_event.type = WM_EVENT_KBD;
            kbd_event.kbd.keycode = event.keycode;
            kbd_event.kbd.pressed = event.pressed;
            kbd_event.kbd.repr = event.repr;
            ringbuffer_write(win->events, sizeof(wm_event_t), (uint8_t*) &kbd_event);

            if (!(win->flags & WM_SKIP_INPUT)) {
                return;
            }
        }
    }
}
