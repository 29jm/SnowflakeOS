#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <kernel/uapi/uapi_syscall.h>
#include <kernel/uapi/uapi_wm.h>
#include <kernel/uapi/uapi_kbd.h>

int32_t syscall(uint32_t eax);
int32_t syscall1(uint32_t eax, uint32_t ebx);
int32_t syscall2(uint32_t eax, uint32_t ebx, uint32_t ecx);

// Sets a magic breakpoint in Bochs on the line it's called.
#define BREAK() do { \
                    asm ("xchgw %bx, %bx\n"); \
                } while (false)

typedef struct {
	char* title;
	uint32_t width;
	uint32_t height;
	fb_t fb;
	uint32_t id;
	uint32_t flags;
} window_t;

void snow_get_fb_info(fb_t* fb);
void snow_sleep(uint32_t ms);
uint32_t snow_get_kernel_mem_usage();

// Drawing functions
void snow_draw_pixel(fb_t fb, int x, int y, uint32_t col);
void snow_draw_rect(fb_t fb, int x, int y, int w, int h, uint32_t col);
void snow_draw_line(fb_t fb, int x0, int y0, int x1, int y1, uint32_t col);
void snow_draw_border(fb_t fb, int x, int y, int w, int h, uint32_t col);
void snow_draw_character(fb_t fb, char c, int x, int y, uint32_t col);
void snow_draw_string(fb_t fb, char* str, int x, int y, uint32_t col);

// GUI functions
window_t* snow_open_window(char* title, int width, int height, uint32_t flags);
void snow_close_window(window_t* win);
void snow_draw_window(window_t* win);
void snow_render_window(window_t* win);
wm_event_t snow_get_event(window_t* win);