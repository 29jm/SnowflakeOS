#include <kernel/fb.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/sys.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static fb_t fb;

/* "fb" stands for "framebuffer" throughout the code.
 */
void init_fb(mb2_t* boot) {
    mb2_tag_fb_t* fb_info = (mb2_tag_fb_t*) mb2_find_tag(boot, MB2_TAG_FB);

    fb.width = fb_info->width;
    fb.height = fb_info->height;
    fb.pitch = fb_info->pitch;
    fb.bpp = fb_info->bpp;

    if (fb.bpp != 32) {
        printke("unsupported bit depth: %d", fb.bpp);
    }

    uintptr_t address = (uintptr_t) fb_info->addr;

    // Remap our framebuffer
    uint32_t size = fb.height*fb.pitch;
    uintptr_t buff = (uintptr_t) kamalloc(size, 0x1000);

    for (uint32_t i = 0; i < size/0x1000; i++) {
        page_t* p = paging_get_page(buff + 0x1000*i, false, 0);
        *p = (address + 0x1000*i) | PAGE_PRESENT | PAGE_RW;
    }

    fb.address = buff;
}

fb_t fb_get_info() {
    return fb;
}