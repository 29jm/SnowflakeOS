#include <ui.h>
#include <snow.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* `lbox` stands for "linear box".
 */

void lbox_on_click(lbox_t* lbox, point_t pos) {
    widget_t* child;

    list_for_each_entry(child, &lbox->children) {
        point_t local = ui_to_child_local(child, pos);

        if (point_in_rect(pos, child->bounds)) {
            if (child->on_click) {
                child->on_click(child, local);
            }
        }
    }
}

void lbox_on_draw(lbox_t* lbox, fb_t fb) {
    widget_t* child;

    list_for_each_entry(child, &lbox->children) {
        if (child->on_draw) {
            child->on_draw(child, fb);
        }
    }
}

void lbox_on_resize(lbox_t* lbox) {
    uint32_t other_direction = lbox->direction == UI_HBOX ?
        UI_VBOX : UI_HBOX;
    uint32_t n_children = 0;
    widget_t* child;

    /* Size elements */

    list_for_each_entry(child, &lbox->children) {
        if (child->flags & other_direction) {
            if (lbox->direction == UI_HBOX) {
                child->bounds.h = lbox->widget.bounds.h;
            } else {
                child->bounds.w = lbox->widget.bounds.w;
            }
        } else if (lbox->direction == UI_HBOX &&
                   child->bounds.h > lbox->widget.bounds.h) {
            printf("[hbox] warning: container too narrow for element\n");
            child->bounds.h = lbox->widget.bounds.h;
        } else if (lbox->direction == UI_VBOX &&
                   child->bounds.w > lbox->widget.bounds.w) {
            printf("[vbox] warning: container too narrow for element\n");
            child->bounds.w = lbox->widget.bounds.w;
        }

        n_children++;
    }

    // Compute the size of EXPAND elements in the container's direction
    int32_t expand_space = lbox->direction == UI_HBOX ?
        lbox->widget.bounds.w : lbox->widget.bounds.h;
    uint32_t n_expand = n_children;

    list_for_each_entry(child, &lbox->children) {
        // TODO: use a min size instead for every child
        if (!(child->flags & lbox->direction)) {
            expand_space -= lbox->direction == UI_HBOX ?
                child->bounds.w : child->bounds.h;
            n_expand--;
        }
    }

    if (expand_space <= 0) {
        printf("[ui] error: container overflow\n");
        return;
    }

    uint32_t size = n_expand ? expand_space / n_expand : 0;

    list_for_each_entry(child, &lbox->children) {
        if (child->flags & lbox->direction) {
            if (lbox->direction == UI_HBOX) {
                child->bounds.w = size;
            } else {
                child->bounds.h = size;
            }
        }
    }

    /* Position elements */

    uint32_t position = 0;

    list_for_each_entry(child, &lbox->children) {
        if (lbox->direction == UI_HBOX) {
            child->bounds.x = position;
        } else {
            child->bounds.y = position;
        }

        position += lbox->direction == UI_HBOX ?
            child->bounds.w : child->bounds.h;
    }

    /* Ask elements to resize their content */

    list_for_each_entry(child, &lbox->children) {
        if (child->on_resize) {
            child->on_resize(child);
        }
    }
}

void lbox_on_mouse_move(lbox_t* lbox, point_t pos) {
    widget_t* child;

    list_for_each_entry(child, &lbox->children) {
        point_t local = ui_to_child_local(child, pos);

        if (point_in_rect(pos, child->bounds)) {
            widget_t* prev_hovered = lbox->child_under_cursor;

            if (child != prev_hovered) {
                if (prev_hovered && prev_hovered->on_mouse_exit) {
                    prev_hovered->on_mouse_exit(prev_hovered);
                }

                if (child->on_mouse_enter) {
                    child->on_mouse_enter(child, local);
                }

                lbox->child_under_cursor = child;
            }

            if (child->on_mouse_move) {
                child->on_mouse_move(child, local);
            }
        }
    }
}

void lbox_on_mouse_enter(lbox_t* lbox, point_t pos) {
    widget_t* child;

    list_for_each_entry(child, &lbox->children) {
        point_t local = ui_to_child_local(child, pos);

        if (point_in_rect(pos, child->bounds)) {
            if (child->on_mouse_enter) {
                child->on_mouse_enter(child, local);
                lbox->child_under_cursor = child;
            }
        }
    }
}

void lbox_on_mouse_exit(lbox_t* lbox) {
    if (lbox->child_under_cursor && lbox->child_under_cursor->on_mouse_exit) {
        lbox->child_under_cursor->on_mouse_exit(lbox->child_under_cursor);
    }
}

void lbox_on_mouse_release(lbox_t* lbox, point_t pos) {
    widget_t* child;

    list_for_each_entry(child, &lbox->children) {
        point_t local = ui_to_child_local(child, pos);

        if (point_in_rect(pos, child->bounds)) {
            if (child->on_mouse_release) {
                child->on_mouse_release(child, local);
            }
        }
    }
}

/* Creates an `lbox_t` with the requested direction, `UI_HBOX` or `UI_VBOX`.
 */
lbox_t* lbox_new(uint32_t direction) {
    lbox_t* lbox = zalloc(sizeof(lbox_t));

    lbox->children = LIST_HEAD_INIT(lbox->children);
    lbox->widget.flags = UI_EXPAND;
    lbox->widget.on_click = (widget_clicked_t) lbox_on_click;
    lbox->widget.on_draw = (widget_draw_t) lbox_on_draw;
    lbox->widget.on_resize = (widget_resize_t) lbox_on_resize;
    lbox->widget.on_mouse_move = (widget_mouse_moved_t) lbox_on_mouse_move;
    lbox->widget.on_mouse_release = (widget_mouse_release_t) lbox_on_mouse_release;
    lbox->widget.on_mouse_enter = (widget_mouse_entered_t) lbox_on_mouse_enter;
    lbox->widget.on_mouse_exit = (widget_mouse_exited_t) lbox_on_mouse_exit;
    lbox->direction = direction;

    return lbox;
}

hbox_t* hbox_new() {
    return lbox_new(UI_HBOX);
}

vbox_t* vbox_new() {
    return lbox_new(UI_VBOX);
}

/* Appends a widget to the container, resizing elements as needed.
 */
void lbox_add(lbox_t* lbox, widget_t* widget) {
    widget->parent = (widget_t*) lbox;

    list_add(&lbox->children, widget);
    lbox_on_resize(lbox);
}

/* Destroys the children of this container. Their `on_free` method is called
 * if present and any reference to them becomes invalid.
 */
void lbox_clear(lbox_t* lbox) {
    while (!list_empty(&lbox->children)) {
        list_t* elem = list_first(&lbox->children);
        widget_t* child = list_entry(elem, widget_t);

        if (child->on_free) {
            child->on_free(child);
        }

        list_del(elem);
        free(child);
    }
}

/* Appends a widget to the right of the last element of the vbox.
 */
void vbox_add(vbox_t* vbox, widget_t* widget) {
    lbox_add(vbox, widget);
}

/* Appends a widget below the last element of the hbox.
 */
void hbox_add(hbox_t* hbox, widget_t* widget) {
    lbox_add(hbox, widget);
}

void vbox_clear(vbox_t* vbox) {
    lbox_clear(vbox);
}

void hbox_clear(hbox_t* hbox) {
    lbox_clear(hbox);
}