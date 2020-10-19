#pragma once

#include <snow.h>

#include <kernel/uapi/uapi_wm.h> // For fb_t

#include <stdint.h>
#include <stdbool.h>

#include "list.h"

#define UI_EXPAND_VERTICAL 1
#define UI_EXPAND_HORIZONTAL 2
#define UI_EXPAND (UI_EXPAND_VERTICAL | UI_EXPAND_HORIZONTAL)

// VBox/HBox
#define UI_VBOX UI_EXPAND_VERTICAL
#define UI_HBOX UI_EXPAND_HORIZONTAL

#define UI_ICON_SIZE (16*16*3)

#define W(w) ((widget_t*) w)

typedef struct {
    int32_t x, y, w, h;
} rect_t;

typedef struct {
    int32_t x, y;
} point_t;

/* Main widget class. Other widgets "inherit" it by having a `widget_t` as first
 * struct member, so that they can be casted as `widget_t`.
 */
typedef struct _widget_t {
    struct _widget_t* parent;
    rect_t bounds;
    uint32_t flags;
    // For passing around data in callbacks
    void* data;
    void (*on_click)(struct _widget_t*, point_t);
    void (*on_mouse_move)(struct _widget_t*, point_t);
    void (*on_draw)(struct _widget_t*, fb_t);
    void (*on_free)(struct _widget_t*);
    void (*on_resize)(struct _widget_t*);
} widget_t;

typedef void (*widget_clicked_t)(widget_t*, point_t);
typedef void (*widget_mouse_moved_t)(widget_t*, point_t);
typedef void (*widget_draw_t)(widget_t*, fb_t);
typedef void (*widget_resize_t)(widget_t*);
typedef void (*widget_freed_t)(widget_t*);

typedef struct {
    fb_t* fb;
    widget_t* root;
} ui_app_t;

/* UI widgets.
 */
typedef struct {
    widget_t widget;
    list_t* children;
    uint32_t direction;
} lbox_t;

typedef lbox_t hbox_t;
typedef lbox_t vbox_t;

typedef struct {
    widget_t widget;
    char* text;
    void (*on_click)();
} button_t;

typedef struct {
    widget_t widget;
    char* title;
    uint8_t* icon;
} titlebar_t;

typedef struct {
    widget_t widget;
    uint32_t color;
    uint32_t* to_set;
} color_button_t;

typedef struct {
    widget_t widget;
    bool is_drawing;
    bool needs_drawing;
    bool needs_clearing;
    point_t last_pos;
    point_t new_pos;
    uint32_t color;
} canvas_t;

bool point_in_rect(point_t p, rect_t r);
ui_app_t ui_app_new(window_t* win, const uint8_t* icon);
void ui_set_root(ui_app_t app, widget_t* widget);
void ui_draw(ui_app_t app);
void ui_handle_input(ui_app_t app, wm_event_t event);
rect_t ui_get_absolute_bounds(widget_t* widget);
point_t ui_to_child_local(widget_t* widget, point_t point);
point_t ui_absolute_to_local(widget_t* widget, point_t point);

hbox_t* hbox_new();
void hbox_add(hbox_t* hbox, widget_t* widget);

vbox_t* vbox_new();
void vbox_add(vbox_t* vbox, widget_t* widget);
void vbox_clear(vbox_t* vbox);

button_t* button_new(char* text);
void button_set_on_click(button_t* button, void (*callback)(button_t*));
void button_set_text(button_t* button, const char* text);

titlebar_t* titlebar_new(const char* title, const uint8_t* icon);

color_button_t* color_button_new(uint32_t color, uint32_t* to_set);

canvas_t* canvas_new();