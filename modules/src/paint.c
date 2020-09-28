#include <snow.h>
#include <ui.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void redraw();
void copy_fb(fb_t src, fb_t dest, uint32_t x, uint32_t y);

typedef struct {
	widget_t widget;
	bool needs_drawing;
	bool needs_clearing;
	point_t last_pos;
} canvas_t;

void canvas_on_mouse_move(canvas_t* canvas, point_t p) {
	(void) canvas;
	(void) p;
	// TODO
}

void canvas_on_draw(canvas_t* canvas, fb_t fb) {
	(void) canvas;
	(void) fb;
}

canvas_t* canvas_new() {
	canvas_t* canvas = calloc(sizeof(canvas_t));

	canvas->widget.flags |= UI_EXPAND;
	canvas->widget.on_draw = (widget_draw_t) canvas_on_draw;

	return canvas;
}

const uint32_t width = 550;
const uint32_t height = 320;
const uint32_t title_height = 20;
const uint32_t fb_width = width - 2;
const uint32_t fb_height = height - title_height - 1;
const uint32_t fb_x = 1;
const uint32_t fb_y = title_height;

window_t* win;
fb_t draw_fb;
rect_t pos;
uint32_t color = 0x000000;
bool running = true;
bool drawing = false;
uint8_t* icon = NULL;

int main() {
	win = snow_open_window("Pisos", width, height, WM_NORMAL);
	draw_fb = (fb_t) {
		.address = (uintptr_t) malloc(fb_width*fb_height*win->fb.bpp/8),
		.pitch = fb_width*win->fb.bpp/8,
		.width = fb_width,
		.height = fb_height,
		.bpp = win->fb.bpp
	};

	icon = calloc(3*16*16);
	FILE* fd = fopen("/pisos_16.rgb", "r");

	if (fd) {
		fread(icon, 3, 16*16, fd);
		fclose(fd);
	}

	// Snow white default
	memset((void*) draw_fb.address, 0xFFFFFF, draw_fb.height*draw_fb.pitch);

	redraw();

	while (running) {
		wm_event_t event = snow_get_event(win);

		if (event.type & WM_EVENT_KBD && event.kbd.keycode == KBD_ESCAPE) {
			running = false;
		}

		if (event.type & WM_EVENT_CLICK) {
			drawing = !drawing;
			pos = (rect_t) {
				.x = event.mouse.position.left, .y = event.mouse.position.top
			};

			pos.x -= fb_x;
			pos.y -= fb_y;
		}

		if (!(event.type & WM_EVENT_MOUSE_MOVE)) {
			continue;
		}

		rect_t npos = (rect_t) {
			.x = event.mouse.position.left, .y = event.mouse.position.top
		};

		int32_t x = npos.x - fb_x;
		int32_t y = npos.y - fb_y;

		if (x < 0 || y < 0 || x >= (int32_t) fb_width || y >= (int32_t) fb_height) {
			continue;
		}

		if (drawing) {
			snow_draw_line(draw_fb, pos.x, pos.y, x, y, color);
		}

		pos.x = x;
		pos.y = y;

		redraw();
	}

	free((void*) draw_fb.address);
	free(icon);
	snow_close_window(win);

	return 0;
}

void redraw() {
	// title bar
	snow_draw_rect(win->fb, 0, 0, win->width, 20, 0x00222221);
	snow_draw_rgb(win->fb, icon, 3, 3, 16, 16, 0xFFFFFF);
	snow_draw_string(win->fb, win->title, 16+2+4, 3, 0x00FFFFFF);
	snow_draw_border(win->fb, 0, 0, win->width, 20, 0x00000000);
	// border of the whole window
	snow_draw_border(win->fb, 0, 0, win->width, win->height, 0x00555555);

	copy_fb(draw_fb, win->fb, fb_x, fb_y);

	snow_render_window(win);
}

/* Copy a whole framebuffer to another, at some specified offset.
 * It must fit, no protections offered at this time.
 */
void copy_fb(fb_t src, fb_t dest, uint32_t x, uint32_t y) {
	uintptr_t d_off = dest.address + y*dest.pitch + x*dest.bpp/8;
	uintptr_t s_off = src.address;

	for (uint32_t i = 0; i < src.height; i++) {
		memcpy((void*) d_off, (void*) s_off, src.pitch);
		d_off += dest.pitch;
		s_off += src.pitch;
	}
}