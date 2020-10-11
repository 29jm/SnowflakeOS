#include <ui.h>
#include <snow.h>

#include <string.h>
#include <stdlib.h>

void button_on_draw(button_t* button, fb_t fb) {
	rect_t r = ui_get_absolute_bounds((widget_t*) button);

	snow_draw_rect(fb, r.x, r.y, r.w, r.h, 0x757575);
	snow_draw_border(fb, r.x, r.y, r.w, r.h, 0x00000);

	uint32_t x = r.x + r.w/2 - strlen(button->text)*8/2;
	uint32_t y = r.y + r.h/2 - 16/2;
	snow_draw_string(fb, button->text, x, y, 0xFFFFFF);
}

void button_on_click(button_t* button, point_t p) {
	(void) p;
	// TODO: show the clicked state graphically

	if (button->on_click) {
		button->on_click(button);
	}
}

void button_on_free(button_t* button) {
	free(button->text);
}

button_t* button_new(char* text) {
	button_t* button = calloc(sizeof(button_t));
	uint32_t margin = 2;

	button->text = strdup(text); // TODO: free this
	button->widget.bounds.w = strlen(text)*8 + 4*margin;
	button->widget.bounds.h = 16 + 2*margin; // 2px margin
	button->widget.on_draw = (widget_draw_t) button_on_draw;
	button->widget.on_click = (widget_clicked_t) button_on_click;

	return button;
}

void button_set_on_click(button_t* button, void (*callback)(button_t*)) {
	button->on_click = callback;
}

void button_set_text(button_t* button, const char* text) {
	if (button->text) {
		free(button->text);
	}

	button->text = strdup(text);
}