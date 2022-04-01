#include <snow.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 30

void set_str(const char* base, const char* end, uint32_t n, char* s) {
    char num[BUF_SIZE];

    strcpy(s, base);
    itoa(n, num, 10);
    strcat(s, num);
    strcat(s, end);
}

int main() {
    window_t* win = snow_open_window("System information", 275, 100, WM_FOREGROUND | WM_SKIP_INPUT);

    char heap_usage[BUF_SIZE];
    char mem_usage[BUF_SIZE];
    char mem_total[BUF_SIZE];

    while (true) {
        wm_event_t evt = snow_get_event(win);

        if (evt.type == WM_EVENT_CLICK) {
            printf("sys_info clicked at %d;%d\n", evt.mouse.position.left, evt.mouse.position.top);
        }

        if (evt.type == WM_EVENT_KBD && evt.kbd.keycode == KBD_ESCAPE) {
            break;
        }

        sys_info_t info;
        syscall2(SYS_INFO, SYS_INFO_MEMORY, (uintptr_t) &info);

        set_str("Kernel heap used: ", "KiB", info.kernel_heap_usage >> 10, heap_usage);
        set_str("Ram used: ", "KiB", info.ram_usage >> 10, mem_usage);
        set_str("Ram total: ", "MiB", info.ram_total >> 20, mem_total);

        snow_draw_window(win); // Draws the title bar and borders
        snow_draw_string(win->fb, heap_usage, 4, 24, 0x00AA1100);
        snow_draw_string(win->fb, mem_usage, 4, 40, 0x00AA1100);
        snow_draw_string(win->fb, mem_total, 4, 56, 0x00AA1100);

        snow_render_window(win);
        snow_sleep(300);
    }

    snow_close_window(win);

    return 0;
}