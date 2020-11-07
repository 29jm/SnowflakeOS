#pragma once

#include <kernel/multiboot.h>

#include <kernel/uapi/uapi_wm.h>

void init_fb(fb_info_t info);

fb_t fb_get_info();