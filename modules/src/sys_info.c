#include <snow.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 30
#define TIME_STEP_MS 500
#define TIME_PERIOD_GRAPHED_MS (60*1000)
#define DATA_POINTS_COUNT (TIME_PERIOD_GRAPHED_MS / TIME_STEP_MS)

static const uint32_t win_w = 350;
static const uint32_t win_h = 175;
static const uint32_t txt_color = 0xCCCCCC;
static const uint32_t ram_color = 0xFF00;
static const uint32_t kheap_color = 0xFF0000;

const uint32_t graph_x0 = 4;
const uint32_t graph_y0 = WM_TB_HEIGHT + 40;
const uint32_t graph_w = win_w - 8;
const uint32_t graph_h = win_h - WM_TB_HEIGHT - 36 - 8;
const float graph_step = ((float) graph_w) / DATA_POINTS_COUNT;

inline void draw_line(window_t* win, uint32_t* data, uint32_t max, uint32_t i, uint32_t neg_off, uint32_t color) {
    snow_draw_line(win->fb,
                   graph_x0+(i-1-neg_off)*graph_step,
                   graph_y0 + (graph_h - graph_h*data[i-1]/max),
                   graph_x0+(i-neg_off)*graph_step,
                   graph_y0 + (graph_h - graph_h*data[i]/max),
                   color);
}

int main() {
    window_t* win = snow_open_window("System information", win_w, win_h, WM_FOREGROUND | WM_SKIP_INPUT);

    char heap_usage[BUF_SIZE];
    char mem_usage[BUF_SIZE];
    uint32_t kheap[DATA_POINTS_COUNT];
    uint32_t ram[DATA_POINTS_COUNT];
    sys_info_t info;
    uint32_t idx = 0; // Where to write the next data point; wraps around
    uint32_t total_updates = 0;

    while (true) {
        wm_event_t evt = snow_get_event(win);

        if (evt.type == WM_EVENT_KBD && evt.kbd.keycode == KBD_ESCAPE) {
            break;
        }

        syscall2(SYS_INFO, SYS_INFO_MEMORY, (uintptr_t) &info);

        /* Update data */
        kheap[idx] = info.kernel_heap_usage;
        ram[idx] = info.ram_usage;
        idx = (idx + 1) % DATA_POINTS_COUNT;
        total_updates += 1;

        /* Update graphics */

        // Title bar and borders
        snow_draw_window(win);

        // Text info: ~20px high (no titlebar)
        sprintf(heap_usage, "Kernel heap: %u/%u KiB", info.kernel_heap_usage >> 10, info.kernel_heap_total >> 10);
        sprintf(mem_usage, "Ram: %u/%u MiB ", info.ram_usage >> 20, info.ram_total >> 20);
        snow_draw_string(win->fb, heap_usage, 4, WM_TB_HEIGHT + 4, txt_color);
        snow_draw_string(win->fb, mem_usage, 4, WM_TB_HEIGHT + 4+16, txt_color);

        // Graph: ~200px high: 40 -> 240

        snow_draw_rect(win->fb, graph_x0, graph_y0, graph_w, graph_h, 0x00FFFFFF); // Background

        if (total_updates < DATA_POINTS_COUNT) {
            for (uint32_t i = 1; i < idx; i++) {
                draw_line(win, ram, info.ram_total, i, 0, ram_color);
                draw_line(win, kheap, info.kernel_heap_total, i, 0, kheap_color);
            }
        } else {
            for (uint32_t i = idx+1; i < DATA_POINTS_COUNT; i++) {
                draw_line(win, ram, info.ram_total, i, idx, ram_color);
                draw_line(win, kheap, info.kernel_heap_total, i, idx, kheap_color);
            }

            // TODO: draw call for the junction between end and start of buffer here

            for (uint32_t i = 1; i < idx; i++) {
                draw_line(win, ram, info.ram_total, i, idx - DATA_POINTS_COUNT, ram_color);
                draw_line(win, kheap, info.kernel_heap_total, i, idx - DATA_POINTS_COUNT, kheap_color);
            }
        }

        // Render everything
        snow_render_window(win);
        snow_sleep(TIME_STEP_MS);
    }

    snow_close_window(win);

    return 0;
}