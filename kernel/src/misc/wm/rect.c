#include <kernel/wm.h>

#include <stdio.h>
#include <stdlib.h>
#include <list.h>

/* Allocates the specified `rect_t` on the stack.
 */
rect_t* rect_new(uint32_t t, uint32_t l, uint32_t b, uint32_t r) {
    rect_t* rect = (rect_t*) kmalloc(sizeof(rect_t));

    *rect = (rect_t) {
        .top = t, .left = l, .bottom = b, .right = r
    };

    return rect;
}

/* Copy a rect on the heap.
 */
rect_t* rect_new_copy(rect_t r) {
    return rect_new(r.top, r.left, r.bottom, r.right);
}

/* Returns a rectangle corresponding to the area spanned by the window.
 */
rect_t rect_from_window(wm_window_t* win) {
    return (rect_t) {
        .top = win->y,
        .left = win->x,
        .bottom = win->y + win->kfb.height - 1,
        .right = win->x + win->kfb.width - 1
    };
}

/* Returns whether two rectangular areas intersect.
 * Note that touching isn't intersecting.
 */
bool rect_intersect(rect_t a, rect_t b) {
    return a.left <= b.right && a.right >= b.left &&
           a.top <= b.bottom && a.bottom >= b.top;
}

/* Pretty-prints a `rect_t`.
 */
void print_rect(rect_t* r) {
    printf("top:%d, left:%d, bottom:%d, right:%d\n",
        r->top, r->left, r->bottom, r->right);
}

/* Removes a rectangle `clip` from the union of `rects` by splitting intersecting
 * rects by `clip`. Frees every discarded rect.
 */
void rect_subtract_clip_rect(list_t* rects, rect_t clip) {
    list_t* iter;
    list_t* n;

    list_for_each_safe(iter, n, rects) {
        rect_t* current = list_entry(iter, rect_t);

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
void rect_add_clip_rect(list_t* rects, rect_t clip) {
    rect_t* r = rect_new_copy(clip);

    rect_subtract_clip_rect(rects, clip);
    list_add_front(rects, r);
}

/* Empties the list while freeing its elements.
 */
void rect_clear_clipped(list_t* rects) {
    while (!list_empty(rects)) {
        kfree(list_first_entry(rects, rect_t));
        list_del(list_first(rects));
    }
}

/* Splits the original rectangle in more rectangles that cover the area
 *     `original \ split`
 * in set-theoric terms. Returns a dynamically allocated list of those
 * dynamically allocated rectangles.
 */
list_t* rect_split_by(rect_t rect, rect_t split) {
    list_t* list = kmalloc(sizeof(list_t));
    rect_t* tmp;

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