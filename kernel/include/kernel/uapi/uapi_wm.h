#pragma once

#include <stdbool.h>

#define WM_NORMAL     0
#define WM_BACKGROUND 1
#define WM_FOREGROUND 2

#define WM_CMD_OPEN   0
#define WM_CMD_CLOSE  1
#define WM_CMD_RENDER 2
#define WM_CMD_INFO   3
#define WM_CMD_EVENT  4

#define WM_EVENT_CLICK 1

typedef struct {
	int32_t top, left, bottom, right;
} rect_t;

typedef struct {
    uintptr_t address;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
} fb_t;

typedef struct {
	uint32_t type;
	rect_t position;
} wm_event_t;

typedef struct {
    fb_t* fb;
    uint32_t flags;
} wm_param_open_t;

typedef struct {
	uint32_t win_id;
	rect_t* clip;
} wm_param_render_t;

typedef struct {
	uint32_t win_id;
	wm_event_t* event;
} wm_param_event_t;