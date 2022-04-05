#include <snow.h>
#include <ui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

typedef struct {
    widget_t widget;
    vbox_t* vbox;
    char* path;
    bool dirty;
} folder_view_t;

void fv_entry_clicked(button_t* btn) {
    folder_view_t* fv = (folder_view_t*) W(btn)->data;
    char* newpath = zalloc(strlen(fv->path) + strlen(btn->text) + 1);

    strcpy(newpath, fv->path);

    if (strcmp(fv->path, "/")) {
        strcat(newpath, "/");
    }

    strcat(newpath, btn->text);

    free(fv->path);
    fv->path = newpath;
    fv->dirty = true;
}

void fv_refresh(folder_view_t* fv) {
    DIR* d = opendir(fv->path);
    struct dirent* dent;

    vbox_clear(fv->vbox);

    while (d && (dent = readdir(d)) != NULL) {
        button_t* btn = button_new(dent->d_name);

        btn->text = strdup(dent->d_name);
        W(btn)->flags |= UI_EXPAND_HORIZONTAL;
        W(btn)->data = fv;

        if (dent->d_type == DENT_DIRECTORY) {
            btn->on_click = fv_entry_clicked;
        } else {
            btn->on_click = NULL;
        }

        vbox_add(fv->vbox, W(btn));
        free(dent);
    }

    if (d) {
        closedir(d);
    }

    fv->dirty = false;
}

void fv_on_draw(folder_view_t* fv, fb_t fb) {
    if (fv->dirty) {
        fv_refresh(fv);
    }

    rect_t r = ui_get_absolute_bounds(W(fv));
    snow_draw_rect(fb, r.x, r.y, r.w, r.h, 0x000000);
    W(fv->vbox)->on_draw(W(fv->vbox), fb);
}

void fv_on_click(folder_view_t* fv, point_t p) {
    W(fv->vbox)->on_click(W(fv->vbox), p);
}

void fv_on_free(folder_view_t* fv) {
    W(fv->vbox)->on_free(W(fv->vbox));
    free(fv->path);
}

void fv_on_resize(folder_view_t* fv) {
    W(fv->vbox)->bounds = W(fv)->bounds;
    W(fv->vbox)->on_resize(W(fv->vbox));
}

folder_view_t* fv_new(const char* path) {
    folder_view_t* fv = zalloc(sizeof(folder_view_t));
    fv->path = strdup(path);
    fv->dirty = true;
    fv->vbox = vbox_new();

    W(fv)->flags = UI_EXPAND;
    W(fv)->on_click = (widget_clicked_t) fv_on_click;
    W(fv)->on_draw = (widget_draw_t) fv_on_draw;
    W(fv)->on_free = (widget_freed_t) fv_on_free;
    W(fv)->on_resize = (widget_resize_t) fv_on_resize;

    return fv;
}

int main() {
    ui_app_t files = ui_app_new("files", 400, 700, NULL);
    bool running = true;

    folder_view_t* fv = fv_new("/");
    ui_set_root(files, W(fv));

    while (running) {
        wm_event_t e = snow_get_event(files.win);
        ui_handle_input(files, e);
        ui_draw(files);
    }

    return 0;
}