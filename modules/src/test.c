#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <snow.h>

int main() {
	fb_info_t fb = snow_get_fb_info();

	fb.address = (uintptr_t) snow_alloc(fb.height*fb.pitch/0x1000);

	for (uint32_t i = 0; i < fb.width*fb.height; i++) {
		((uint32_t*) fb.address)[i] = i | i*512 | i % 512;
	}

	snow_draw_rect(fb, 256, 192, 512, 768/2, 0xFFFFFFFF);
	snow_draw_rect(fb, 256+128, 192+96, 256, 768/4, 0xFF000000);
	snow_draw_pixel(fb, 512, 768/2, 0x00FFFF00);
	snow_draw_line(fb, 256+128, 192+96, 256+128+256, 192+96+768/4, 0x0000FF00);
	snow_draw_line(fb, 256+128, 192+96+768/4, 256+128+256, 192+96, 0x0000FF00);
	snow_draw_border(fb, 256+128, 192+96, 256, 768/4, 0x0000F000);

	snow_render((uintptr_t) fb.address);

	while (true);

	return 0;
}
