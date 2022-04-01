#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <kernel/uapi/uapi_syscall.h>
#include <kernel/uapi/uapi_wm.h>
#include <kernel/uapi/uapi_kbd.h>

int32_t syscall(uint32_t eax);
int32_t syscall1(uint32_t eax, uint32_t ebx);
int32_t syscall2(uint32_t eax, uint32_t ebx, uint32_t ecx);
int32_t syscall3(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

// Sets a magic breakpoint in Bochs on the line it's called.
#define BREAK() do { \
                    asm ("xchgw %bx, %bx\n"); \
                } while (false)

// all this stuff needs to be here so then both window_t and ui.h can access it
#define UI_TB_HEIGHT 30
#define UI_TB_PADDING 8

typedef struct {
    int64_t bg_color, // background color
    base_color, border_color, text_color, highlight, // title bar colors
    border_color2; // window border color
} color_scheme_t;

#define UI_DEFAULT_COLOR &(color_scheme_t){\
    .base_color = 0x757575,\
    .border_color = 0x000000,\
    .border_color2 = 0x121212,\
    .text_color = 0xFFFFFF,\
    .highlight = 0x030303,\
}

typedef struct {
    char* title;
    uint32_t width;
    uint32_t height;
    fb_t fb;
    uint32_t id;
    uint32_t flags;
    color_scheme_t* color;
} window_t;

void snow_get_fb_info(fb_t* fb);
void snow_sleep(uint32_t ms);

// Drawing functions
void snow_draw_pixel(fb_t fb, int x, int y, uint32_t col);
void snow_draw_rect(fb_t fb, int x, int y, int w, int h, uint32_t col);
void snow_draw_line(fb_t fb, int x0, int y0, int x1, int y1, uint32_t col);
void snow_draw_border(fb_t fb, int x, int y, int w, int h, uint32_t col);
void snow_draw_character(fb_t fb, char c, int x, int y, uint32_t col);
void snow_draw_string(fb_t fb, char* str, int x, int y, uint32_t col);
void snow_draw_rgba(fb_t fb, uint32_t* rgba, int x, int y, int w, int h);
void snow_draw_rgb(fb_t fb, uint8_t* rgb, int x, int y, int w, int h);
void snow_draw_rgb_masked(fb_t fb, uint8_t* rgb, int x, int y, int w, int h, uint32_t mask);

// GUI functions
window_t* snow_open_window(const char* title, int width, int height, uint32_t flags);
void snow_close_window(window_t* win);
void snow_draw_window(window_t* win);
void snow_render_window(window_t* win);
void snow_render_window_partial(window_t* win, wm_rect_t clip);
wm_event_t snow_get_event(window_t* win);