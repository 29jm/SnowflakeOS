#include <snow.h>

/* Fills the passed struct with the display's buffer information.
 * Note: the `address` field returned is garbage.
 */
void snow_get_fb_info(fb_t* fb) {
	syscall2(SYS_WM, WM_CMD_INFO, (uint32_t) fb);
}

void snow_sleep(uint32_t ms) {
	syscall1(SYS_SLEEP, ms);
}