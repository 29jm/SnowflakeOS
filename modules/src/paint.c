#include <snow.h>

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
	// TODO
}

void canvas_on_draw(canvas_t canvas, fb_t fb) {

}

canvas_t* canvas_new() {
	canvas_t* canvas = calloc(sizeof(canvas_t));

	canvas->widget.flags |= UI_EXPAND;
	canvas->widget.on_draw = (widget_draw_t) canvas_on_draw;
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

int main() {
    win = snow_open_window("Pisos - pissing on snow", width, height, WM_NORMAL);
    draw_fb = (fb_t) {
        .address = (uintptr_t) malloc(fb_width*fb_height*win->fb.bpp/8),
        .pitch = fb_width*win->fb.bpp/8,
        .width = fb_width,
        .height = fb_height,
        .bpp = win->fb.bpp
    };

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
            pos = event.mouse.position;
            pos.left -= fb_x;
            pos.top -= fb_y;
        }

        if (!(event.type & WM_EVENT_MOUSE_MOVE)) {
            continue;
        }

        rect_t npos = event.mouse.position;
        uint32_t x = npos.left - fb_x;
        uint32_t y = npos.top - fb_y;

        if (drawing && x > 0 && x < fb_width-1 && y > 0 && y < fb_height-1) {
            snow_draw_line(draw_fb, pos.left, pos.top, x, y, color);
        }

        pos.left = x;
        pos.top = y;

        redraw();
    }

    free((void*) draw_fb.address);
    snow_close_window(win);

    return 0;
}

void redraw() {
	// title bar
	snow_draw_rect(win->fb, 0, 0, win->width, 20, 0x00222221);
	snow_draw_border(win->fb, 0, 0, win->width, 20, 0x00000000);
	snow_draw_string(win->fb, win->title, 4, 3, 0x00FFFFFF);
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