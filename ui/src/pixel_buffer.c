#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ui.h>

void pixel_buffer_on_draw(pixel_buffer_t* pb, fb_t fb) {
    rect_t r = ui_get_absolute_bounds(W(pb));
    snow_draw_rgba(fb, pb->pixels, r.x, r.y, pb->width, pb->height);
}

pixel_buffer_t* pixel_buffer_new() {
    pixel_buffer_t* pixbuf = zalloc(sizeof(pixel_buffer_t));

    W(pixbuf)->flags = UI_EXPAND;
    W(pixbuf)->on_draw = (widget_draw_t) pixel_buffer_on_draw;

    return pixbuf;
}

void pixel_buffer_draw(pixel_buffer_t* pb, uint32_t* pixels, uint32_t width, uint32_t height) {
    if (width > (uint32_t) W(pb)->bounds.w || height > (uint32_t) W(pb)->bounds.h) {
        printf("pixel_buffer: buffer exceeds widget dimensions\n");
        return;
    }

    pb->pixels = pixels;
    pb->width = width;
    pb->height = height;
}