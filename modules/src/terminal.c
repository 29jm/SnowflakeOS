#include <snow.h>
#include <ui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ui.h>

typedef struct {
    char* buf;
    uint32_t buf_len;
    uint32_t len;
} str_t;

void redraw(str_t* text_buf, const str_t* input_buf);
void draw_cursor(uint32_t x, uint32_t y);
str_t* str_new(const char* str);
void str_free(str_t* str);
void str_append(str_t* str, const char* text);
void interpret_cmd(str_t* text_buf, str_t* cmd);
uint32_t count_lines(str_t* str);
char* scroll_view(char* str);

const uint32_t twidth = 550;
const uint32_t theight = 600;
const uint32_t char_width = 8;
const uint32_t char_height = 16;
const uint32_t max_col = twidth / char_width - 1;
const uint32_t max_line = theight / char_height - 1;

const char* prompt = "snowflakeos $ ";
const uint32_t margin = UI_DEFAULT_PADDING;
const uint32_t text_color = 0xE0E0E0;
const float cursor_blink_time = 1;

ui_app_t term;
bool cursor = true;
bool running = true;
bool focused = true;

int main() {
    term = ui_app_new("Terminal", twidth, theight, NULL);
    titlebar_t* tb = list_entry(((vbox_t*) term.root)->children.next, titlebar_t);

    syscall(SYS_MAKETTY);

    str_t* text_buf = str_new(prompt);
    str_t* input_buf = str_new("");
    cursor = true;

    uint32_t last_time = 0;
    bool last_tb_hovered = false;

    redraw(text_buf, input_buf);

    while (running) {
        wm_event_t event = snow_get_event(term.win);
        wm_kbd_event_t key = event.kbd;
        bool needs_redrawing = false;

        ui_handle_input(term, event);

        // Hack to avoid useless redraws
        if (tb->hovered != last_tb_hovered) {
            needs_redrawing = true;
            last_tb_hovered = tb->hovered;
        }

        // Do we have focus?
        if (event.type == WM_EVENT_GAINED_FOCUS) {
            focused = true;
            needs_redrawing = true;
        } else if (event.type == WM_EVENT_LOST_FOCUS) {
            focused = false;
            cursor = false;
            needs_redrawing = true;
        }

        // Time & cursor blinks
        if (focused) {
            sys_info_t info;
            syscall2(SYS_INFO, SYS_INFO_UPTIME, (uintptr_t) &info);

            uint32_t time = (uint32_t) (info.uptime / cursor_blink_time);

            if (time != last_time) {
                last_time = time;
                cursor = !cursor;
                needs_redrawing = true;
            }
        }

        // Print things that have been output, if any, and append a prompt
        const uint32_t buf_size = 256;
        char buf[buf_size];
        uint32_t read;
        bool anything_read = false;

        while ((read = fread(buf, 1, buf_size - 1, stdout))) {
            buf[read] = '\0';
            str_append(text_buf, buf);
            needs_redrawing = true;
            anything_read = true;
        }

        if (anything_read) {
            str_append(text_buf, prompt);
        }

        if (event.type == WM_EVENT_KBD && event.kbd.pressed) {
            needs_redrawing = true;

            switch (key.keycode) {
            case KBD_ENTER:
            case KBD_KP_ENTER:
                str_append(text_buf, input_buf->buf);
                interpret_cmd(text_buf, input_buf);
                printf("\n");
                input_buf->buf[0] = '\0';
                input_buf->len = 0;
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
        }

        if (needs_redrawing) {
            redraw(text_buf, input_buf);
        }
    }

    str_free(text_buf);
    str_free(input_buf);

    ui_app_destroy(term);

    return 0;
}

void redraw(str_t* text_buf, const str_t* input_buf) {
    /* Window decorations */
    snow_draw_rect(term.win->fb, 0, 0, term.win->width, term.win->height, 0x353535);
    snow_draw_border(term.win->fb, 0, 0, term.win->width, term.win->height, 0x404040);
    term.root->on_draw(term.root, term.win->fb);

    uint32_t y = WM_TB_HEIGHT + 4; // below title bar

    // Temporarily concatenate the input and a cursor
    str_append(text_buf, input_buf->buf);

    if (cursor) {
        str_append(text_buf, "_");
    }

    char line_buf[max_col + 1];
    char* text_view = text_buf->buf;
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
        } else {
            strncpy(line_buf, text_view, max_col);
            line_buf[max_col] = '\0';
            text_view += max_col;
        }

        snow_draw_string(term.win->fb, line_buf, margin, y, text_color);

        y += char_height;
    }

    // De-concatenate
    text_buf->buf[text_buf->len - input_buf->len] = '\0';
    text_buf->len -= input_buf->len;

    if (cursor) {
        text_buf->buf[text_buf->len - 1] = '\0';
        text_buf->len -= 1;
    }

    // Update the window
    snow_render_window(term.win);
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

void interpret_cmd(str_t* text_buf, str_t* input_buf) {
    if (!strcmp(input_buf->buf, "exit")) {
        running = false;
        return;
    }

    if (!strcmp(input_buf->buf, "dmesg")) {
        sys_info_t info;
        info.kernel_log = malloc(4096);
        syscall2(SYS_INFO, SYS_INFO_LOG, (uintptr_t) &info);
        str_append(text_buf, info.kernel_log);
        free(info.kernel_log);
        return;
    }

    char* cmd = input_buf->buf;
    uint32_t n_args = 0;
    char** args = NULL;
    char* next = cmd;

    while (*next) {
        args = realloc(args, (++n_args + 1) * sizeof(char*));

        while (isspace(*next)) {
            next++;
        }

        uint32_t n = strchrnul(next, ' ') - next;
        args[n_args - 1] = strndup(next, n);
        args[n_args] = NULL;

        next = strchrnul(next, ' ');
    }

    if (!args) {
        return;
    }

    int32_t ret = syscall2(SYS_EXEC, (uintptr_t) args[0], (uintptr_t) args);

    while (args && *args) {
        free(*args);
        args++;
    }

    free(args);

    if (ret != 0) {
        str_append(text_buf, "\ninvalid command: ");
        str_append(text_buf, cmd);
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
