#include <snow.h>

#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>

#define MAX_WIN 0
#define BUF_SIZE 512

window_t* make_win(char* title) {
	window_t* win = snow_open_window(title, 250, 120, WM_NORMAL);
	char id_str[5];
	itoa(win->id, id_str, 10);

	snow_draw_window(win); // Draws the title bar and borders
	snow_draw_string(win->fb, id_str, 175, 55, 0x00AA1100);

	snow_render_window(win);

	return win;
}

void tree(char* path, int level) {
	static char p[256] = "\0";
	DIR* d = NULL;
	struct dirent* e = NULL;

	if (p[0] == 0) {
		strcpy(p, path);
	}

	d = opendir(p);

	while (d && (e = readdir(d))) {
		for (int i = 0; i < level; i++) {
			printf("  ");
		}

		printf("- %s%s\n", e->d_name, e->d_type == 2 ? "/" : "");

		if (e->d_type == 2 && e->d_name[0] != '.') {
			int nl = strlen(e->d_name);
			strcpy(p + strlen(p), "/");
			strcpy(p + strlen(p), e->d_name);
			tree(p, level + 1);
			p[strlen(p) - nl - 1] = 0;
		}

		free(e);
	}

	closedir(d);
}

int main() {
	char buf[BUF_SIZE];

	printf("Treeing /:\n");
	tree("/", 0);

	DIR* root = opendir("/");
	FILE* hello = fopen("/motd", "r");

	fclose(hello);
	closedir(root);

	while (true);

	for (int i = 0; i < MAX_WIN; i++) {
		if (i == 0) {
			make_win(buf);
		} else {
			make_win("doom.exe (not responding)");
		}
	}

	return 0;
}