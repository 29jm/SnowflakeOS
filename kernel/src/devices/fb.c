#include <kernel/fb.h>
#include <kernel/paging.h>
#include <kernel/mem.h>
#include <kernel/pmm.h>
#include <kernel/sys.h>

#include <string.h>
#include <stdio.h>

static fb_t fb;

/* "fb" stands for "framebuffer" throughout the code.
 */
void init_fb(fb_info_t fb_info) {
	fb = (fb_t) {
		.width = fb_info.width,
		.height = fb_info.height,
		.pitch = fb_info.pitch,
		.bpp = fb_info.bpp
	};

	if (fb.bpp != 32) {
		printf("[FB] Unsupported bit depth: %d\n", fb.bpp);
	}

	uintptr_t address = (uintptr_t) fb_info.address;

	// Remap our framebuffer
	uint32_t size = fb.height*fb.pitch;
	uintptr_t buff = (uintptr_t) kamalloc(size, 0x1000);

	for (uint32_t i = 0; i < size/0x1000; i++) {
		page_t* p = paging_get_page(buff + 0x1000*i, false, 0);
		*p = (address + 0x1000*i) | PAGE_PRESENT | PAGE_RW;
	}

	fb.address = buff;
}

/* Draws the content of `buff` to the screen.
 * `buff` must be of the size returned by `fb_get_size`.
 */
void fb_render(fb_t buff) {
	if (buff.height*buff.pitch != fb.height*fb.pitch) {
		return;
	}

	memcpy((void*) fb.address, (void*) buff.address, buff.height*buff.pitch);
}

fb_t fb_get_info() {
	return fb;
}