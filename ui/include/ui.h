#pragma once

#include <snow.h>

#include <kernel/uapi/uapi_wm.h> // For fb_t

#include <stdint.h>
#include <stdbool.h>
#include <list.h>

#define W(w) ((widget_t*) w)
#define UI_ICON_SIZE (16*16*3)

// Generic widget flags
#define UI_EXPAND_VERTICAL 1
#define UI_EXPAND_HORIZONTAL 2
#define UI_EXPAND (UI_EXPAND_VERTICAL | UI_EXPAND_HORIZONTAL)

// Flags for lbox.c
#define UI_VBOX UI_EXPAND_VERTICAL
#define UI_HBOX UI_EXPAND_HORIZONTAL

// Recommended paddings and margins
#define UI_DEFAULT_PADDING 8

typedef struct {
    int32_t x, y, w, h;
} rect_t;

/* Defines the colors of an application, can (and should) be respected by its
 * widgets.
 * A color scheme can be defined per-app by assigning `app.root->color`, or per
 * widget in the same way. In both cases, child widgets inherit the color
 * schemes of the closest parent that defines one.
 * TODO: Investigate most relevant fields to keep and what to add.
 *       Convert the fields to `uint32_t`.
 */
typedef struct {
    uint64_t bg_color, base_color, border_color;
    uint64_t text_color, highlight, border_color2;
} color_scheme_t;

/* Generic type representing an ui component, like a button, a text field...
 * Contains the properties common to all of those: dimensions, hints on how to
 * size them, their place in the hierarchy, and callbacks for the operations
 * relevant to them.
 */
typedef struct widget_t {
    /* Every widget but the root has a parent container */
    struct widget_t* parent;
    color_scheme_t* color;
    /* Bounds of the widget, relative to its parent */
    rect_t bounds;
    uint32_t flags;
    /* For passing around data in callbacks, unused by the toolkit itself */
    void* data;
    /* Callbacks, to be set by widget implementations when relevant */
    void (*on_click)(struct widget_t*, point_t);
    void (*on_mouse_move)(struct widget_t*, point_t);
    void (*on_mouse_release)(struct widget_t*, point_t);
    void (*on_draw)(struct widget_t*, fb_t);
    void (*on_free)(struct widget_t*);
    void (*on_resize)(struct widget_t*);
} widget_t;

typedef void (*widget_clicked_t)(widget_t*, point_t);
typedef void (*widget_mouse_moved_t)(widget_t*, point_t);
typedef void (*widget_mouse_release_t)(widget_t*, point_t);
typedef void (*widget_draw_t)(widget_t*, fb_t);
typedef void (*widget_resize_t)(widget_t*);
typedef void (*widget_freed_t)(widget_t*);

/* Represents the Ui of an application, in the sense that it holds the root widget.
 * Created with `ui_app_new`.
 * Note: using this structure and related functions is not strictly necessary in
 * order to use the ui toolkit, it's just convenient.
 */
typedef struct {
    window_t* win;
    widget_t* root;
} ui_app_t;

/* UI widget types: they all 'inherit' the widget structure by having a widget
 * type as first member. This way, pointers to specialized widgets can safely
 * be cast to `widget_t`, allowing for some genericity.
 * A macro is defined to shorten this common cast, `W(object)`. */

/* Linear box, a container that holds widgets in the horizontal or vertical
 * direction. Prefer using `vbox_ŧ` or `hbox_t` and related functions instead
 * of `lbox_t`, which implements them. */
typedef struct {
    widget_t widget;
    list_t children;
    uint32_t direction;
} lbox_t;

typedef lbox_t hbox_t;
typedef lbox_t vbox_t;

typedef struct {
    widget_t widget;
    char* text;
    bool is_clicked;
    void (*on_click)();
    void (*on_release)();
} button_t;


typedef struct {
    widget_t widget;
    uint32_t color;
    uint32_t* to_set;
    bool is_clicked;
} color_button_t;

typedef struct {
    widget_t widget;
    char* title;
    uint8_t* icon;
} titlebar_t;

typedef struct {
    widget_t widget;
    bool is_drawing;
    bool needs_drawing;
    bool needs_clearing;
    point_t last_pos;
    point_t new_pos;
    uint32_t color;
} canvas_t;

typedef struct {
    widget_t widget;
    uint32_t* pixels;
    uint32_t width;
    uint32_t height;
} pixel_buffer_t;

static color_scheme_t default_color_scheme = {
    .base_color = 0x757575,
    .border_color = 0x000000,
    .border_color2 = 0x121212,
    .text_color = 0xFFFFFF,
    .highlight = 0x030303,
};

bool point_in_rect(point_t p, rect_t r);
ui_app_t ui_app_new(const char* title, uint32_t width, uint32_t height, const uint8_t* icon);
void ui_app_destroy(ui_app_t app);
void ui_set_root(ui_app_t app, widget_t* widget);
void ui_set_title(ui_app_t app, const char* title);
void ui_draw(ui_app_t app);
void ui_handle_input(ui_app_t app, wm_event_t event);
rect_t ui_get_absolute_bounds(widget_t* widget);
point_t ui_to_child_local(widget_t* widget, point_t point);
point_t ui_absolute_to_local(widget_t* widget, point_t point);

color_scheme_t* ui_get_color_scheme(widget_t* widget);
uint32_t ui_shade_color(uint32_t c, int32_t shade_shift);

hbox_t* hbox_new();
void hbox_add(hbox_t* hbox, widget_t* widget);

vbox_t* vbox_new();
void vbox_add(vbox_t* vbox, widget_t* widget);
void vbox_clear(vbox_t* vbox);

button_t* button_new(char* text);
void button_set_on_click(button_t* button, void (*callback)(button_t*));
void button_set_text(button_t* button, const char* text);

titlebar_t* titlebar_new(const char* title, const uint8_t* icon);
void titlebar_set_title(titlebar_t* tb, const char* title);

color_button_t* color_button_new(uint32_t color, uint32_t* to_set);

canvas_t* canvas_new();

pixel_buffer_t* pixel_buffer_new();
void pixel_buffer_draw(pixel_buffer_t* pb, uint32_t* pixels, uint32_t width, uint32_t height);