#include <snow.h>
#include <ui.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void redraw();
void copy_fb(fb_t src, fb_t dest, uint32_t x, uint32_t y);

typedef struct {
	widget_t widget;
	bool is_drawing;
	bool needs_drawing;
	bool needs_clearing;
	point_t last_pos;
	point_t new_pos;
	uint32_t color;
} canvas_t;

void canvas_on_click(canvas_t* canvas, point_t p) {
	rect_t bounds = ui_get_absolute_bounds((widget_t*) canvas);
	canvas->is_drawing = !canvas->is_drawing;

	if (canvas->is_drawing) {
		canvas->last_pos = (point_t) { bounds.x + p.x, bounds.y + p.y };
		canvas->new_pos = canvas->last_pos;
	}
}

void canvas_on_mouse_move(canvas_t* canvas, point_t p) {
	rect_t bounds = ui_get_absolute_bounds((widget_t*) canvas);

	canvas->needs_drawing = true;
	canvas->last_pos = canvas->new_pos;
	canvas->new_pos = (point_t) { p.x+bounds.x, p.y+bounds.y };
}

void canvas_on_draw(canvas_t* canvas, fb_t fb) {
	if (canvas->needs_clearing) {
		canvas->needs_clearing = false;
		rect_t bounds = ui_get_absolute_bounds((widget_t*) canvas);
		snow_draw_rect(fb, bounds.x, bounds.y, bounds.w, bounds.h, 0xFFFFFF);
	}

	if (canvas->is_drawing && canvas->needs_drawing) {
		canvas->needs_drawing = false;

		snow_draw_line(fb, canvas->last_pos.x, canvas->last_pos.y,
			canvas->new_pos.x, canvas->new_pos.y, canvas->color);
	}
}

canvas_t* canvas_new() {
	canvas_t* canvas = calloc(sizeof(canvas_t));

	canvas->widget.flags = UI_EXPAND;
	canvas->widget.on_click = (widget_clicked_t) canvas_on_click;
	canvas->widget.on_draw = (widget_draw_t) canvas_on_draw;
	canvas->widget.on_mouse_move = (widget_mouse_moved_t) canvas_on_mouse_move;
	canvas->needs_clearing = true;

	return canvas;
}

// callbacks
void on_clear_clicked(button_t* button, point_t p);

const uint32_t width = 550;
const uint32_t height = 320;

const uint32_t colors[] = {
	0x363537, 0x0CCE6B, 0xDCED31, 0xEF2D56, 0xED7D3A, 0x4C1A57,
	0xFF3CC7, 0xF0F600, 0x00E5E8, 0x007C77
};

window_t* win;
canvas_t* canvas;
uint32_t color = 0x000000;
bool running = true;
uint8_t* icon = NULL;

int main() {
	win = snow_open_window("Pisos", width, height, WM_NORMAL);
	icon = calloc(3*16*16);
	FILE* fd = fopen("/pisos_16.rgb", "r");

	if (fd) {
		fread(icon, 3, 16*16, fd);
		fclose(fd);
	}

	ui_app_t paint = ui_app_new(win, fd ? icon : NULL);

	vbox_t* vbox = vbox_new();
	ui_set_root(paint, (widget_t*) vbox);

	hbox_t* menu = hbox_new();
	menu->widget.flags &= ~UI_EXPAND_VERTICAL;
	menu->widget.bounds.h = 26;
	vbox_add(vbox, (widget_t*) menu);

	canvas = canvas_new();
	vbox_add(vbox, (widget_t*) canvas);

	for (uint32_t i = 0; i < sizeof(colors)/sizeof(colors[0]); i++) {
		color_button_t* cbutton = color_button_new(colors[i], &canvas->color);
		hbox_add(menu, (widget_t*) cbutton);
	}

	button_t* button = button_new("Clear");
	button->on_click = on_clear_clicked;
	hbox_add(menu, (widget_t*) button);

	while (running) {
		wm_event_t event = snow_get_event(win);

		if (event.type & WM_EVENT_KBD && event.kbd.keycode == KBD_ESCAPE) {
			running = false;
		}

		ui_handle_input(paint, event);
		ui_draw(paint);
		snow_render_window(win);
	}

	free(icon);
	snow_close_window(win);

	return 0;
}

void on_clear_clicked(button_t* button, point_t p) {
	(void) button;
	(void) p;

	canvas->needs_clearing = true;
}