#include <snow.h>

/* Fills the passed struct with the display's buffer information.
 * Note: the `address` field returned is garbage.
 */
void snow_get_fb_info(fb_t* fb) {
	asm volatile (
		"mov $6, %%eax\n"
		"mov %[fb], %%ecx\n"
		"int $0x30\n"
		:: [fb] "r" (fb)
		: "%eax"
	);
}