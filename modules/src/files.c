#include <snow.h>
#include <ui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

typedef struct {
	vbox_t list;
	char* path;
	bool dirty;
} folder_view_t;

void folder_view_update(folder_view_t* fv);

void folder_view_on_click(button_t* entry) {
	folder_view_t* fv = (folder_view_t*) W(entry)->data;

	if (strlen(fv->path) + strlen(entry->text) >= 256) {
		return;
	}

	strcat(fv->path, entry->text);
	folder_view_update(fv);
}

void folder_view_update(folder_view_t* fv) {
	DIR* d = opendir(fv->path);
	vbox_clear(&fv->list);

	struct dirent* e = NULL;

	while (d && (e = readdir(d))) {
		button_t* entry = NULL;
		char name[256] = "";

		strcpy(name, e->d_name);

		if (e->d_type == 2) {
			strcat(name, "/");
			entry = button_new(name);
			entry->on_click = folder_view_on_click;
		} else {
			entry = button_new(name);
		}

		W(entry)->flags = UI_EXPAND_HORIZONTAL;
		W(entry)->data = fv;

		vbox_add(&fv->list, (widget_t*) entry);
		free(e);
	}

	if (d) {
		closedir(d);
	}
}

void folder_view_on_free(folder_view_t* fv) {
	vbox_clear(&fv->list);
	free(fv->path);
}

void folder_view_on_draw(folder_view_t* fv, fb_t fb) {
	if (fv->dirty) {
		folder_view_update(fv);
		fv->dirty = false;
	}

	rect_t r = ui_get_absolute_bounds(W(fv));

	snow_draw_rect(fb, r.x, r.y, r.w, r.h, 0x000000);
	((widget_draw_t) W(fv)->data)(W(fv), fb);
}

folder_view_t* folder_view_new(const char* path) {
	folder_view_t* fv = calloc(sizeof(folder_view_t));
	fv->dirty = true;
	fv->path = calloc(256);
	strcpy(fv->path, path);
	vbox_t* vb = vbox_new();
	fv->list = *vb;
	free(vb);

	W(fv)->data = W(fv)->on_draw;
	W(fv)->on_free = (widget_freed_t) folder_view_on_free;
	W(fv)->on_draw = (widget_draw_t) folder_view_on_draw;

	return fv;
}

void refresh(char* p, vbox_t* vb);
void folder_clicked(button_t* entry);

char path[256] = "/";
vbox_t* file_list;

int main() {
	window_t* win = snow_open_window("Files", 400, 247, WM_NORMAL);
	ui_app_t files = ui_app_new(win, NULL);
	bool running = true;

	folder_view_t* fv = folder_view_new("/");
	ui_set_root(files, W(fv));

	while (running) {
		wm_event_t e = snow_get_event(win);
		ui_handle_input(files, e);
		ui_draw(files);
		snow_render_window(win);
	}

	return 0;
}

void folder_clicked(button_t* entry) {
	strcat(path, entry->text);
	refresh(path, file_list);
}

void refresh(char* p, vbox_t* vb) {
	DIR* d = opendir(p);
	vbox_clear(vb);

	struct dirent* e = NULL;

	while (d && (e = readdir(d))) {
		button_t* entry = NULL;
		char name[256] = "";

		strcpy(name, e->d_name);
		printf("%s\n", name);

		if (e->d_type == 2) {
			strcat(name, "/");
			entry = button_new(name);
			entry->on_click = folder_clicked;
		} else {
			entry = button_new(name);
		}

		entry->widget.flags = UI_EXPAND_HORIZONTAL;

		vbox_add(vb, (widget_t*) entry);
		free(e);
	}

	if (d) {
		closedir(d);
	}
}