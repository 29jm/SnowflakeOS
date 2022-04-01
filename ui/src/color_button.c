#include <ui.h>
#include <snow.h>

#include <stdlib.h>
#include <math.h>

void color_button_on_click(color_button_t* button, point_t p) {
    (void) p;
    button->is_clicked = true;
    *button->to_set = button->color;
}

void color_button_on_draw(color_button_t* button, fb_t fb) {
    rect_t r = ui_get_absolute_bounds((widget_t*) button);
    color_scheme_t* clr = get_widget_color(&button->widget);

    if (button->is_clicked) {
        snow_draw_rect(fb, r.x, r.y, r.w, r.h, button->color + clr->highlight);
    } else {
        snow_draw_rect(fb, r.x, r.y, r.w, r.h, button->color);
    }
}

void color_button_on_release(color_button_t* button, point_t p) {
    button->is_clicked = false;
}

void color_button_on_resize(color_button_t* button) {
    uint32_t s = fmin(button->widget.bounds.w, button->widget.bounds.h);
    button->widget.bounds.w = s;
    button->widget.bounds.h = s;
}

/* returns a new button of color `color` that when clicked sets `to_set` to
 * its given color
 */
color_button_t* color_button_new(uint32_t color, uint32_t* to_set) {
    color_button_t* button = zalloc(sizeof(color_button_t));

    button->color = color;
    button->to_set = to_set;

    button->widget.flags = UI_EXPAND_VERTICAL;
    button->widget.on_click = (widget_clicked_t) color_button_on_click;
    button->widget.on_mouse_release = (widget_mouse_release_t) color_button_on_release;
    button->widget.on_draw = (widget_draw_t) color_button_on_draw;
    button->widget.on_resize = (widget_resize_t) color_button_on_resize;
    button->widget.bounds.w = 26;
    button->widget.bounds.h = 26;

    return button;
}