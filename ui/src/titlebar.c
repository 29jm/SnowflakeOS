#include <ui.h>
#include <snow.h>

#include <string.h>
#include <stdlib.h>

void titlebar_on_draw(titlebar_t* tb, fb_t fb) {
    rect_t r = ui_get_absolute_bounds((widget_t*) tb);

    snow_draw_rect(fb, r.x, r.y, r.w, r.h, 0x303030);

    if (tb->icon) {
        snow_draw_rgb_masked(fb, tb->icon, r.x+2, r.y+2, 16, 16, 0xFFFFFF);
        snow_draw_string(fb, tb->title, r.x+16+5, r.y+3, 0xFFFFFF);
    } else {
        snow_draw_string(fb, tb->title, r.x+3, r.y+3, 0xFFFFFF);
    }

    snow_draw_border(fb, r.x, r.y, r.w, r.h, 0x404040);
}

void titlebar_on_free(titlebar_t* tb) {
    if (tb->icon) {
        free(tb->icon);
    }

    free(tb->title);
}

titlebar_t* titlebar_new(const char* title, const uint8_t* icon) {
    titlebar_t* tb = zalloc(sizeof(titlebar_t));

    tb->widget.flags = UI_EXPAND_HORIZONTAL;
    tb->widget.bounds.h = 20;
    tb->widget.on_draw = (widget_draw_t) titlebar_on_draw;
    tb->widget.on_free = (widget_freed_t) titlebar_on_free;
    tb->title = strdup(title);

    if (icon) {
        tb->icon = malloc(UI_ICON_SIZE);
        memcpy(tb->icon, icon, UI_ICON_SIZE);
    }

    return tb;
}

void titlebar_set_title(titlebar_t* tb, const char* title) {
    if (tb->title) {
        free(tb->title);
    }

    tb->title = strdup(title);
}