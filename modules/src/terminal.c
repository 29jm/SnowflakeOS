#include <snow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char* buf;
	uint32_t buf_len;
	uint32_t len;
} str_t;

void redraw();
void draw_cursor(uint32_t x, uint32_t y);
str_t* str_new(const char* str);
void str_free(str_t* str);
void str_append(str_t* str, const char* text);
void interpret_cmd(char* cmd);
uint32_t count_lines(str_t* str);
char* scroll_view(char* str);

const uint32_t twidth = 400;
const uint32_t theight = 254;
const uint32_t char_width = 8;
const uint32_t char_height = 16;
const uint32_t max_col = twidth / char_width - 1;
const uint32_t max_line = theight / char_height - 1;

const char* prompt = "snowflakeos $ ";
const uint32_t margin = 1;
const uint32_t text_color = 0xE0E0E0;

window_t* win;
str_t* text_buf;
str_t* input_buf;

int main() {
	win = snow_open_window("Terminal", twidth, theight, WM_NORMAL);

	text_buf = str_new(prompt);
	input_buf = str_new("");

	redraw();

	while (true) {
		wm_event_t event = snow_get_event(win);
		wm_kbd_event_t key = event.kbd;

		// Redraw only on input
		if (!(event.type & WM_EVENT_KBD && event.kbd.pressed)) {
			continue;
		}

		switch (event.kbd.keycode) {
			case KBD_ENTER:
			case KBD_KP_ENTER:
				str_append(text_buf, input_buf->buf);
				str_append(text_buf, "\n");
				interpret_cmd(input_buf->buf);
				input_buf->buf[0] = '\0';
				input_buf->len = 0;
				str_append(text_buf, prompt);
				break;
			case KBD_BACKSPACE:
				if (input_buf->len) {
					input_buf->buf[input_buf->len - 1] = '\0';
					input_buf->len -= 1;
				}
				break;
			default:
				if (key.keycode < KBD_KP_ENTER) {
					char str[2] = "\0\0";
					str[0] = key.repr;
					str_append(input_buf, str);
				}
				break;
		}

		redraw();
	}

	str_free(text_buf);
	str_free(input_buf);

	snow_close_window(win);
}

void redraw() {
	/* Window decorations */

	// background
	snow_draw_rect(win->fb, 0, 0, win->width, win->height, 0x00353535);
	// title bar
	snow_draw_rect(win->fb, 0, 0, win->width, 20, 0x00222221);
	snow_draw_border(win->fb, 0, 0, win->width, 20, 0x00000000);
	snow_draw_string(win->fb, win->title, 4, 3, 0x00FFFFFF);
	// border of the whole window
	snow_draw_border(win->fb, 0, 0, win->width, win->height, 0x00555555);

	/* Text content */

	uint32_t y = 22; // below title bar

	// Temporarily concatenate the input and a cursor
	str_append(text_buf, input_buf->buf);
	str_append(text_buf, "_");

	char* text_view = text_buf->buf;
	char* line_buf = malloc(max_col + 1);
	uint32_t n_lines = count_lines(text_buf);

	// Scroll the buffer as needed
	if (n_lines > max_line) {
		for (uint32_t i = 0; i < n_lines - max_line; i++) {
			text_view = scroll_view(text_view);
		}
	}

	// Draw line by line, wrapping text
	while (text_view < &text_buf->buf[text_buf->len]) {
		char* lf = strchrnul(text_view, '\n');
		uint32_t line_len = (uint32_t) (lf - text_view);

		if (line_len <= max_col) {
			strncpy(line_buf, text_view, line_len);
			line_buf[line_len] = '\0';
			text_view += line_len + 1; // Discard the line feed
			// x = line_len * char_width;
		} else {
			strncpy(line_buf, text_view, max_col);
			line_buf[max_col] = '\0';
			text_view += max_col;
		}

		snow_draw_string(win->fb, line_buf, margin, y, text_color);

		y += char_height;
	}

	// De-concatenate
	text_buf->buf[text_buf->len - input_buf->len - 1] = '\0';
	text_buf->len -= input_buf->len + 1;

	// Update the window
	snow_render_window(win);
}

str_t* str_new(const char* str) {
	str_t* s = malloc(sizeof(str_t));
	uint32_t size = strlen(str) + 1;

	s->buf = malloc(size);
	s->buf_len = size;
	s->len = size - 1;

	strcpy(s->buf, str);

	return s;
}

void str_free(str_t* str) {
	free(str->buf);
	free(str);
}

void str_append(str_t* str, const char* text) {
	uint32_t prev_len = str->len;
	uint32_t needed = strlen(text) + prev_len + 1;

	if (needed > str->buf_len) {
		char* new_buf = malloc(2*needed);

		strcpy(new_buf, str->buf);
		free(str->buf);

		str->buf = new_buf;
		str->buf_len = 2*needed;
	}

	strcpy(&str->buf[prev_len], text);
	str->len = needed - 1;
}

void interpret_cmd(char* cmd) {
	if (!strcmp(cmd, "uname")) {
		str_append(text_buf, "SnowflakeOS 0.4\n");
	} else if (!strcmp(cmd, "ls")) {
		str_append(text_buf, "I do not know of this \"filesystem\" you speak of. Get off my lawn.\n");
	} else {
		int32_t ret = syscall1(SYS_EXEC, (uintptr_t) cmd);

		if (ret != 0) {
			str_append(text_buf, "invalid command: \"");
			str_append(text_buf, cmd);
			str_append(text_buf, "\"\n");
		}
	}
}

uint32_t count_lines(str_t* str) {
	char* text_view = str->buf;
	uint32_t n_lines = 0;

	while (text_view < &str->buf[str->len]) {
		char* lf = strchrnul(text_view, '\n');
		uint32_t line_len = (uint32_t) (lf - text_view);

		if (line_len <= max_col) {
			text_view += line_len + 1; // Discard the line feed
		} else {
			text_view += max_col;
		}

		n_lines += 1;
	}

	return n_lines;
}

/* Discards the first line of the buffer.
 */
char* scroll_view(char* str) {
	char* lf = strchrnul(str, '\n');
	uint32_t line_len = (uint32_t) (lf - str);

	if (line_len <= max_col) {
		return lf + 1;
	}

	return str + max_col;
}