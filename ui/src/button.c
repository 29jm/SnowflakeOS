#include <ui.h>
#include <snow.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

color_scheme_t btn_def = {
    .base_color = 0x757575,
    .border_color = 0x00000,
    .text_color = 0xFFFFFF,
    .highlight = 0x777777,
};

void button_on_draw(button_t* button, fb_t fb) {
    color_scheme_t* clr = button->clr;
    rect_t r = ui_get_absolute_bounds((widget_t*) button);

    if (button->text != NULL) {
        if (button->is_clicked) {
            snow_draw_rect(fb, r.x, r.y, r.w, r.h, clr->highlight);
        } else {
            snow_draw_rect(fb, r.x, r.y, r.w, r.h, clr->base_color);
        }
        snow_draw_border(fb, r.x, r.y, r.w, r.h, clr->border_color);

        uint32_t x = r.x + r.w/2 - strlen(button->text) * 8 / 2;
        uint32_t y = r.y + r.h/2 - 16/2;
        snow_draw_string(fb, button->text, x, y, clr->text_color);
    } else {
        snow_draw_rect(fb, r.x, r.y, r.w, r.h, clr->base_color);
    }
}

void button_on_click(button_t* button, point_t p) {
    rect_t r = ui_get_absolute_bounds((widget_t*) button);

    if (point_in_rect(p, r)) {
        button->is_clicked = true;
    } else button->is_clicked = false;
}

void button_on_free(button_t* button) {
    free(button->text);
}

button_t* button_new(char* text, color_scheme_t* clr) {
    button_t* button = zalloc(sizeof(button_t));
    uint32_t margin = tb_padding;

    if (clr) {
        button->clr = clr;
    } else {
        button->clr = &btn_def;
    }

    button->text = strdup(text);
    button->is_clicked = false;
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