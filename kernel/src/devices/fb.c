#include <kernel/fb.h>

#include <string.h>
#include <stdio.h>

static fb_info_t fb;

void init_fb(fb_info_t fb_info) {
    fb = fb_info;

    if (fb.bpp != 32) {
        printf("[FB] Unsupported bit depth: %d\n", fb.bpp);
    }

    if (fb.address != (uint32_t) fb.address) {
        printf("[FB] Unsupported framebuffer address: too high\n");
    }
}

/* Draws the content of `buffer` to the screen.
 * `buffer` must be of the size returned by `fb_get_size`.
 */
void fb_render(uintptr_t buffer) {
    memcpy((void*) (uintptr_t) fb.address, (void*) buffer, fb_get_size());
}

fb_info_t fb_get_info() {
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