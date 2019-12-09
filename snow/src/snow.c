#include <snow.h>

/* Allocates `n` pages of memory located after the program's memory.
 * Returns a pointer to the first allocated page.
 * Note: prefer using `malloc` or `aligned_alloc`.
 */
void* snow_alloc(uint32_t n) {
	uintptr_t address;

	asm (
		"mov $4, %%eax\n"
		"mov %[n], %%ebx\n"
		"int $0x30\n"
		"mov %%eax, %[address]\n"
		: [address] "=r" (address)
		: [n] "r" (n)
		: "%eax");

	return (void*) address;
}

/* Displays the specified framebuffer to the screen.
 */
void snow_render(fb_t fb) {
	asm (
		"mov $5, %%eax\n"
		"mov %[buffer], %%ebx\n"
		"int $0x30\n"
		:: [buffer] "r" (fb.address)
		: "%eax", "%ebx");
}


/* Returns information about the display.
 * Note: does not allocate a framebuffer, the `address` field will contain
 * garbage.
 */
fb_t snow_get_fb_info() {
	fb_t fb;

	asm (
		"mov $6, %%eax\n"
		"int $0x30\n"
		"mov %%ebx, %[width]\n"
		"mov %%eax, %[pitch]\n"
		"mov %%ecx, %[height]\n"
		"mov %%dl, %[bpp]\n"
		: [pitch] "=r" (fb.pitch),
		  [width] "=r" (fb.width),
		  [height] "=r" (fb.height),
		  [bpp] "=r" (fb.bpp)
		:: "%eax");

	return fb;
}