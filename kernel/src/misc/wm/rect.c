#include <kernel/wm.h>
#include <kernel/sys.h>

#include <stdlib.h>
#include <list.h>

/* Allocates the specified `wm_rect_t` on the stack.
 */
wm_rect_t* rect_new(uint32_t t, uint32_t l, uint32_t b, uint32_t r) {
    wm_rect_t* rect = (wm_rect_t*) kmalloc(sizeof(wm_rect_t));

    *rect = (wm_rect_t) {
        .top = t, .left = l, .bottom = b, .right = r
    };

    return rect;
}

/* Copy a rect on the heap.
 */
wm_rect_t* rect_new_copy(wm_rect_t r) {
    return rect_new(r.top, r.left, r.bottom, r.right);
}

/* Returns a rectangle corresponding to the area spanned by the window.
 */
wm_rect_t rect_from_window(wm_window_t* win) {
    return (wm_rect_t) {
        .top = win->pos->y,
        .left = win->pos->x,
        .bottom = win->pos->y + win->kfb.height - 1,
        .right = win->pos->x + win->kfb.width - 1
    };
}

/* Returns whether two rectangular areas intersect.
 * Note that touching isn't intersecting.
 */
bool rect_intersect(wm_rect_t a, wm_rect_t b) {
    return a.left <= b.right && a.right >= b.left &&
           a.top <= b.bottom && a.bottom >= b.top;
}

/* Pretty-prints a `wm_rect_t`.
 */
void print_rect(wm_rect_t* r) {
    printk("top:%d, left:%d, bottom:%d, right:%d",
        r->top, r->left, r->bottom, r->right);
}

/* Removes a rectangle `clip` from the union of `rects` by splitting intersecting
 * rects by `clip`. Frees every discarded rect.
 */
void rect_subtract_clip_rect(list_t* rects, wm_rect_t clip) {
    list_t* iter;
    list_t* n;

    list_for_each_safe(iter, n, rects) {
        wm_rect_t* current = list_entry(iter, wm_rect_t);

        if (rect_intersect(*current, clip)) {
            list_t* splits = rect_split_by(*current, clip);

            // Remove the newly-split rect from our clipping rects
            list_del(iter);
            kfree(current);

            // Add in what remains of it after splitting
            list_splice(splits, rects);
            kfree(splits);
        }
    }
}

/* Add a clipping rectangle to the area spanned by `rects` by splitting
 * intersecting rects by `clip`.
 */
void rect_add_clip_rect(list_t* rects, wm_rect_t clip) {
    wm_rect_t* r = rect_new_copy(clip);

    rect_subtract_clip_rect(rects, clip);
    list_add_front(rects, r);
}

/* Empties the list while freeing its elements.
 */
void rect_clear_clipped(list_t* rects) {
    while (!list_empty(rects)) {
        kfree(list_first_entry(rects, wm_rect_t));
        list_del(list_first(rects));
    }
}

/* Splits the original rectangle in more rectangles that cover the area
 *     `original \ split`
 * in set-theoric terms. Returns a dynamically allocated list of those
 * dynamically allocated rectangles.
 */
list_t* rect_split_by(wm_rect_t rect, wm_rect_t split) {
    list_t* list = kmalloc(sizeof(list_t));
    wm_rect_t* tmp;

    *list = LIST_HEAD_INIT(*list);

    // Split by the left edge
    if (split.left >= rect.left && split.left <= rect.right) {
        tmp = rect_new(rect.top, rect.left, rect.bottom, split.left - 1);
        list_add(list, tmp);
        rect.left = split.left;
    }

    // Split by the top edge
    if (split.top >= rect.top && split.top <= rect.bottom) {
        tmp = rect_new(rect.top, rect.left, split.top - 1, rect.right);
        list_add(list, tmp);
        rect.top = split.top;
    }

    // Split by the right edge
    if (split.right >= rect.left && split.right <= rect.right) {
        tmp = rect_new(rect.top, split.right + 1, rect.bottom, rect.right);
        list_add(list, tmp);
        rect.right = split.right;
    }

    // Split by the bottom edge
    if (split.bottom >= rect.top && split.bottom <= rect.bottom) {
        tmp = rect_new(split.bottom + 1, rect.left, rect.bottom, rect.right);
        list_add(list, tmp);
        rect.bottom = split.bottom;
    }

    return list;
}