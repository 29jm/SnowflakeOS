#include <kernel/fb.h>

#include <string.h>
#include <stdio.h>

static fb_t fb;

void init_fb(fb_info_t fb_info) {
    fb = (fb_t) {
		.address = fb_info.address,
		.width = fb_info.width,
		.height = fb_info.height,
		.pitch = fb_info.pitch,
		.bpp = fb_info.bpp
	};

    if (fb.bpp != 32) {
        printf("[FB] Unsupported bit depth: %d\n", fb.bpp);
    }

    if (fb.address != (uint32_t) fb.address) {
        printf("[FB] Unsupported framebuffer address: too high\n");
    }
}

/* Draws the content of `buff` to the screen.
 * `buff` must be of the size returned by `fb_get_size`.
 */
void fb_render(fb_t buff) {
    if (buff.height*buff.pitch != fb_get_size()) {
        return;
    }

    uint32_t* offset = (uint32_t*) fb.address;
    memcpy(offset, (void*) buff.address, buff.height*buff.pitch);
}

fb_t fb_get_info() {
    return fb;
}

uintptr_t fb_get_address() {
    return (uintptr_t) fb.address;
}

void fb_set_address(uintptr_t buff) {
    fb.address = buff;
}

/* Returns the size of the framebuffer in bytes.
 */
uint32_t fb_get_size() {
    return fb.pitch*fb.height;
}