#include <snow.h>
#include <ui.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// callbacks
void on_clear_clicked();
void on_save_clicked();

bool load(const char* path, uint32_t w, uint32_t h);
void save();

const uint32_t width = 550;
const uint32_t height = 320;

const uint32_t colors[] = {
    0x363537, 0x0CCE6B, 0xDCED31, 0xEF2D56, 0xED7D3A, 0x4C1A57,
    0xFF3CC7, 0xF0F600, 0x00E5E8, 0x007C77
};

ui_app_t paint;
canvas_t* canvas;
uint32_t color;
bool running = true;
uint8_t* icon = NULL;

int main(int argc, char* argv[]) {
    icon = zalloc(3*16*16);
    color = colors[0];
    FILE* fd = fopen("/pisos_16.rgb", "r");

    if (fd) {
        fread(icon, 3, 16*16, fd);
        fclose(fd);
    }

    paint = ui_app_new("pisos", width, height, fd ? icon : NULL);

    free(icon);

    vbox_t* vbox = vbox_new();
    ui_set_root(paint, W(vbox));

    hbox_t* menu = hbox_new();
    menu->widget.flags &= ~UI_EXPAND_VERTICAL;
    menu->widget.bounds.h = 20;
    vbox_add(vbox, W(menu));

    canvas = canvas_new();
    canvas->color = colors[0];
    vbox_add(vbox, W(canvas));

    for (uint32_t i = 0; i < sizeof(colors)/sizeof(colors[0]); i++) {
        color_button_t* cbutton = color_button_new(colors[i], &canvas->color);
        hbox_add(menu, W(cbutton));
    }

    button_t* button = button_new("Clear");
    button->on_click = on_clear_clicked;
    hbox_add(menu, W(button));

    button_t* save_button = button_new("Save");
    save_button->on_click = on_save_clicked;
    hbox_add(menu, W(save_button));

    ui_draw(paint);

    if (argc == 4) {
        char* fname = argv[1];

        uint32_t w = strtol(argv[2], NULL, 10);
        uint32_t h = strtol(argv[3], NULL, 10);

        if (load(argv[1], w, h)) {
            printf("paint: opened '%s'\n", fname);
        } else {
            printf("paint: failed to load '%s'\n", fname);
        }
    }

    while (running) {
        wm_event_t event = snow_get_event(paint.win);

        if (!event.type) {
            continue;
        }

        if (event.type == WM_EVENT_KBD && event.kbd.keycode == KBD_ESCAPE) {
            running = false;
        }

        ui_handle_input(paint, event);
        ui_draw(paint);
    }

    ui_app_destroy(paint);

    return 0;
}

void on_clear_clicked() {
    canvas->needs_clearing = true;
}

void on_save_clicked() {
    save(NULL);
}

bool load(const char* path, uint32_t w, uint32_t h) {
    FILE* fd = fopen(path, "r");

    if (!fd) {
        return false;
    }

    uint8_t* buf = malloc(w*h*3);

    if ((uint32_t) fread(buf, 3, w*h, fd) < w*h) {
        free(buf);
        return false;
    } else {
        rect_t r = ui_get_absolute_bounds(W(canvas));
        snow_draw_rgb(paint.win->fb, buf, r.x, r.y, w, h);
    }

    free(buf);
    fclose(fd);

    return true;
}

void save() {
    char fname[256] = "drawing_";
    rect_t r = ui_get_absolute_bounds(W(canvas));

    itoa(r.w, fname+strlen(fname), 10);
    strcat(fname, "x");
    itoa(r.h, fname+strlen(fname), 10);
    strcat(fname, ".rgb");

    printf("paint: saving to '%s'.\n", fname);

    FILE* fd = fopen(fname, "w");

    if (!fd) {
        printf("paint: failed to open output file '%s'\n", fname);
        return;
    }

    uint8_t* buf = malloc(r.w*r.h*3);
    fb_t* fb = &paint.win->fb;

    /* Convert from RGBA to RGB */
    for (int y = r.y; y < r.y+r.h; y++) {
        for (int x = r.x; x < r.x+r.w; x++) {
            uint32_t px = ((uint32_t*) fb->address)[y * fb->pitch / 4 + x];
            uint8_t* pixbuf = (uint8_t*) &px;
            uint32_t idx = (y - r.y)*3*r.w + (x - r.x)*3;
            buf[idx] = pixbuf[2];
            buf[idx + 1] = pixbuf[1];
            buf[idx + 2] = pixbuf[0];
        }
    }

    fwrite(buf, 3, r.w*r.h, fd);
    free(buf);
    fclose(fd);
}