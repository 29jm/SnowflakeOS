#ifndef FB_H
#define FB_H

#include <kernel/multiboot.h>

/* Framebuffer type.
 */
typedef struct {
	uintptr_t address;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
} fb_t;

void init_fb(fb_info_t info);
void fb_render(fb_t buff);

fb_t fb_get_info();

#endif