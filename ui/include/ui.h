#pragma once

#include <snow.h>

#include <kernel/uapi/uapi_wm.h> // For fb_t

#include <stdint.h>
#include <stdbool.h>

#include "list.h"

#define UI_EXPAND_VERTICAL 1
#define UI_EXPAND_HORIZONTAL 2
#define UI_EXPAND (UI_EXPAND_VERTICAL | UI_EXPAND_HORIZONTAL)

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
	void (*on_click)(struct _widget_t*, point_t);
	void (*on_mouse_move)(struct _widget_t*);
	void (*on_draw)(struct _widget_t*, fb_t);
	void (*on_free)(struct _widget_t*);
	void (*on_resize)(struct _widget_t*);
} widget_t;

typedef void (*widget_clicked_t)(widget_t*, point_t);
typedef void (*widget_mouse_moved_t)(widget_t*);
typedef void (*widget_draw_t)(widget_t*, fb_t);
typedef void (*widget_resize_t)(widget_t*);

typedef struct {
	fb_t* fb;
	widget_t* root;
} ui_app_t;

/* UI widgets.
 */
typedef struct {
	widget_t widget;
	list_t* children;
} hbox_t;

typedef struct {
	widget_t widget;
	list_t* children;
} vbox_t;

typedef struct {
	widget_t widget;
	char* text;
	void (*on_click)();
} button_t;

typedef struct {
	widget_t widget;
	char* title;
} titlebar_t;

typedef struct {
	widget_t widget;
	uint32_t color;
	uint32_t* to_set;
} color_button_t;

bool point_in_rect(point_t p, rect_t r);
ui_app_t ui_app_new(window_t* win);
void ui_set_root(ui_app_t app, widget_t* widget);
// widget_t* ui_get_root(ui_app_t app);
void ui_draw(ui_app_t app);
void ui_handle_input(ui_app_t app, wm_event_t event);
rect_t ui_get_absolute_bounds(widget_t* widget);
point_t ui_to_child_local(widget_t* widget, point_t point);
point_t ui_absolute_to_local(widget_t* widget, point_t point);

void hbox_on_click(hbox_t* hbox, point_t pos);
void hbox_on_draw(hbox_t* hbox, fb_t fb);
hbox_t* hbox_new();
void hbox_add(hbox_t* hbox, widget_t* widget);
void hbox_free(hbox_t* hbox);

void vbox_on_click(vbox_t* vbox, point_t pos);
void vbox_on_draw(vbox_t* vbox, fb_t fb);
vbox_t* vbox_new();
void vbox_add(vbox_t* vbox, widget_t* widget);
void vbox_free(vbox_t* vbox);

button_t* button_new(char* text);
void button_set_on_click(button_t* button, void (*callback)(button_t*));
void button_set_text(button_t* button, const char* text);

titlebar_t* titlebar_new(const char* title);

color_button_t* color_button_new(uint32_t color, uint32_t* to_set);