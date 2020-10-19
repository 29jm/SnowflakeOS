#include <ui.h>
#include <snow.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

bool point_in_rect(point_t p, rect_t r) {
    return p.x >= r.x && p.x < r.x + r.w && p.y >= r.y && p.y < r.y + r.h;
}

void ui_set_root(ui_app_t app, widget_t* widget) {
    vbox_t* root = (vbox_t*) app.root;

    vbox_add(root, widget);
}

/* Returns a pointer to the box below the window's titlebar.
 */
ui_app_t ui_app_new(window_t* win, const uint8_t* icon) {
    ui_app_t app;

    vbox_t* vbox = vbox_new();
    vbox->widget.bounds = (rect_t) {
        0, 0, win->width, win->height
    };

    titlebar_t* tb = titlebar_new(win->title, icon);

    vbox_add(vbox, (widget_t*) tb);

    app.fb = &win->fb;
    app.root = (widget_t*) vbox;

    return app;
}

void ui_draw(ui_app_t app) {
    app.root->on_draw(app.root, *app.fb);
}

void ui_handle_input(ui_app_t app, wm_event_t event) {
    if (event.type & WM_EVENT_CLICK) {
        point_t pos = { event.mouse.position.left, event.mouse.position.top };

        if (app.root->on_click) {
            app.root->on_click(app.root, pos);
        }
    }

    if (event.type & WM_EVENT_MOUSE_MOVE) {
        point_t pos = { event.mouse.position.left, event.mouse.position.top };

        if (app.root->on_mouse_move) {
            app.root->on_mouse_move(app.root, pos);
        }
    }
}

rect_t ui_get_absolute_bounds(widget_t* widget) {
    rect_t r = widget->bounds;

    for (widget_t* p = widget->parent; p != NULL; p = p->parent) {
        r.x += p->bounds.x;
        r.y += p->bounds.y;
    }

    return r;
}

point_t ui_to_child_local(widget_t* widget, point_t point) {
    return (point_t) {
        .x = point.x - widget->bounds.x,
        .y = point.y - widget->bounds.y
    };
}

point_t ui_absolute_to_local(widget_t* widget, point_t point) {
    if (!widget->parent) {
        return point;
    }

    point_t p = ui_to_child_local(widget, point);

    return ui_absolute_to_local(widget->parent, p);
}