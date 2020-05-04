#pragma once

#include <stdbool.h>

#define WM_NORMAL     0
#define WM_BACKGROUND 1
#define WM_FOREGROUND 2

#define WM_CMD_OPEN   0
#define WM_CMD_CLOSE  1
#define WM_CMD_RENDER 2
#define WM_CMD_INFO   3

typedef struct {
	uint32_t top, left, bottom, right;
} rect_t;

typedef struct {
    uintptr_t address;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
} fb_t;

typedef struct {
    fb_t* fb;
    uint32_t flags;
} wm_param_open_t;