#pragma once

#include <kernel/multiboot.h>

#include <kernel/uapi/uapi_wm.h>

void init_fb(fb_info_t info);
void fb_render(fb_t buff);
void fb_partial_render(fb_t buff, rect_t rect);

fb_t fb_get_info();