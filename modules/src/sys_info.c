#include <snow.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
	window_t* win = snow_open_window("System information", 175, 60, WM_FOREGROUND);

	// printf("sysinfo: ufb at %d\n", win->fb.address);
	uint32_t mem = 0;
	char* str = malloc(16);

	while (1) {
		uint32_t new_mem = snow_get_kernel_mem_usage();

		if (new_mem == mem) {
			syscall(SYS_YIELD);
			continue;
		}

		mem = new_mem;

		itoa(mem/1024, str, 10);

		int n = strlen(str);

		str[n] = ' ';
		str[n+1] = 'K';
		str[n+2] = 'i';
		str[n+3] = 'B';
		str[n+4] = '\0';

		snow_draw_window(win); // Draws the title bar and borders
		snow_draw_string(win->fb, "Kernel mem usage:", 4, 24, 0x00AA1100);
		snow_draw_string(win->fb, str, 4, 40, 0x00AA1100);

		snow_render_window(win);
	}

	return 0;
}