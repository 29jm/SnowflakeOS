#pragma once

#include <kernel/wm.h>

#define WM_NORMAL     0
#define WM_BACKGROUND 1
#define WM_FOREGROUND 2

#define WM_CMD_OPEN   0
#define WM_CMD_CLOSE  1
#define WM_CMD_RENDER 2
#define WM_CMD_INFO   3

typedef struct {
    fb_t* fb;
    uint32_t flags;
} wm_param_open_t;