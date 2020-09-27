#include <snow.h>

#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>

#define MAX_WIN 3
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

	printf("p");
	d = opendir(p);
	printf("b");

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

	tree("/", 0);

	DIR* root = opendir("/");

	if (root) {
		printf("[test] opened / as fd %d\n", root->fd);
		struct dirent* ent = NULL;
		while ((ent = readdir(root))) {
			printf("- %s (inode %d, type %d)\n", ent->d_name, ent->d_ino, ent->d_type);
			if (ent->d_type == 1) {
				buf[0] = '/';
				strcpy(buf+1, ent->d_name);
				buf[1+strlen(ent->d_name)] = 0;
				FILE* f = fopen(buf, "r");
				printf("  opened %s as fd %d, ", buf, f->fd);
				fread(buf, 300, 1, f);
				printf("content: %s\n", buf);
				fclose(f);
				buf[1] = 0;
			}
		}
	}

	FILE* hello = fopen("/hello", "r");

	if (hello) {
		printf("[test] opened file /hello\n");
		int c = EOF;
		int i = 0;

		while ((c = fgetc(hello)) != EOF && i < BUF_SIZE) {
			buf[i++] = c;
		}

		buf[i] = 0;

		printf("[test] content: %s\n", buf);
	}

	fclose(hello);
	closedir(root);

	for (int i = 0; i < MAX_WIN; i++) {
		if (i == 0) {
			make_win(buf);
		} else {
			make_win("doom.exe (not responding)");
		}
	}

	while (1) { }

	return 0;
}