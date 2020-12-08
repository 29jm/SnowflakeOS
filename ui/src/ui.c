#include <ui.h>
#include <snow.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

/* Is that point in that rect?
 * Note: see `ui_get_absolute_bounds` and friends if coordinate conversion is
 * needed.
 */
bool point_in_rect(point_t p, rect_t r) {
    return p.x >= r.x && p.x < r.x + r.w && p.y >= r.y && p.y < r.y + r.h;
}

/* Sets the widget to be displayed below the titlebar of an app created with
 * `ui_app_new`. Usually, this will be a container such as a `vbox_t` or `hbox_t`.
 */
void ui_set_root(ui_app_t app, widget_t* widget) {
    vbox_t* root = (vbox_t*) app.root;

    vbox_add(root, widget);
}

/* Creates the base widget structure of a GUI app, bundled in `ui_app_t` form.
 * This means a titlebar with an optional icon, within a vbox, whose second
 * element will be the user-facing root widget.
 * `icon`, if not null, should point to an RGB array of size 16x16px, 3 bytes
 * per pixel.
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

/* Draws the app to its window's framebuffer. The user is expected to call
 * `snow_draw_window` afterwards to actually render anything to the screen.
 */
void ui_draw(ui_app_t app) {
    app.root->on_draw(app.root, *app.fb);
}

/* Updates the UI according to the event passed in. Must be called in the
 * application's main loop, fed by events obtained through a call to
 * `snow_get_event`. Or by fake events, whatever.
 */
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

/* Get a widget's bounds in absolute coordinates, i.e. relative to the window's
 * bounds.
 * Note: the widget must be a descendant of a root widget.
 */
rect_t ui_get_absolute_bounds(widget_t* widget) {
    rect_t r = widget->bounds;

    for (widget_t* p = widget->parent; p != NULL; p = p->parent) {
        r.x += p->bounds.x;
        r.y += p->bounds.y;
    }

    return r;
}

/* Converts a point from `widget`'s parent coordinates to `widget`'s coordinate
 * system.
 */
point_t ui_to_child_local(widget_t* widget, point_t point) {
    return (point_t) {
        .x = point.x - widget->bounds.x,
        .y = point.y - widget->bounds.y
    };
}

/* Converts a point in absolute coordinates, i.e. relative to the window's bounds,
 * to coordinates local to the given widget.
 * Note: works even if the point isn't within the widget's bounds.
 */
point_t ui_absolute_to_local(widget_t* widget, point_t point) {
    if (!widget->parent) {
        return point;
    }

    point_t p = ui_to_child_local(widget, point);

    return ui_absolute_to_local(widget->parent, p);
}