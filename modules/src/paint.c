#include <snow.h>
#include <ui.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// callbacks
void on_clear_clicked(button_t* button);

const uint32_t width = 550;
const uint32_t height = 320;

const uint32_t colors[] = {
	0x363537, 0x0CCE6B, 0xDCED31, 0xEF2D56, 0xED7D3A, 0x4C1A57,
	0xFF3CC7, 0xF0F600, 0x00E5E8, 0x007C77
};

window_t* win;
canvas_t* canvas;
uint32_t color;
bool running = true;
uint8_t* icon = NULL;

int main() {
	win = snow_open_window("Pisos", width, height, WM_NORMAL);
	icon = zalloc(3*16*16);
	color = colors[0];
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
	menu->widget.bounds.h = 20;
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

void on_clear_clicked(button_t* button) {
	(void) button;

	canvas->needs_clearing = true;
}