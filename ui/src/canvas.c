#include <ui.h>
#include <stdlib.h>

void canvas_on_click(canvas_t* canvas, point_t p) {
    rect_t bounds = ui_get_absolute_bounds((widget_t*) canvas);

    canvas->is_drawing = true;
    canvas->last_pos = (point_t) { bounds.x + p.x, bounds.y + p.y };
    canvas->new_pos = canvas->last_pos;
}

void canvas_on_mouse_move(canvas_t* canvas, point_t p) {
    rect_t bounds = ui_get_absolute_bounds((widget_t*) canvas);

    canvas->needs_drawing = true;
    canvas->last_pos = canvas->new_pos;
    canvas->new_pos = (point_t) { p.x+bounds.x, p.y+bounds.y };
}

void canvas_on_mouse_release(canvas_t* canvas, point_t p) {
    (void) p;
    canvas->is_drawing = false;
}

void canvas_on_mouse_exited(canvas_t* canvas) {
    canvas->is_drawing = false;
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
    canvas_t* canvas = zalloc(sizeof(canvas_t));

    canvas->widget.flags = UI_EXPAND;
    canvas->widget.on_click = (widget_clicked_t) canvas_on_click;
    canvas->widget.on_draw = (widget_draw_t) canvas_on_draw;
    canvas->widget.on_mouse_move = (widget_mouse_moved_t) canvas_on_mouse_move;
    canvas->widget.on_mouse_exit = (widget_mouse_exited_t) canvas_on_mouse_exited;
    canvas->widget.on_mouse_release = (widget_mouse_release_t) canvas_on_mouse_release;
    canvas->needs_clearing = true;

    return canvas;
}