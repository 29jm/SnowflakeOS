#ifndef FB_H
#define FB_H

#include <kernel/multiboot.h>

void init_fb(fb_info_t info);
void fb_render(uintptr_t buffer);

fb_info_t fb_get_info();
uintptr_t fb_get_address();
void fb_set_address(uintptr_t buffer);
uint32_t fb_get_size();

#endif