#pragma once

#include <kernel/multiboot2.h>
#include <kernel/uapi/uapi_wm.h>

void init_fb(mb2_t* boot);

fb_t fb_get_info();