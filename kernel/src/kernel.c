#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/multiboot.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/timer.h>

void kernel_main(multiboot* boot, uint32_t magic) {
	init_term();
	init_gdt();
	init_idt();
	init_irq();
	init_timer();

	printf("Welcome to ");
	term_setcolor(COLOR_CYAN, COLOR_BLACK);
	printf("SnowflakeOS");
	term_setcolor(COLOR_WHITE, COLOR_BLACK);
	printf(" -1.0 !\n\n");

	uint32_t time = 0;
	while (1) {
		uint32_t ntime = timer_get_time();
		if (ntime != time) {
			time = ntime;
			term_change_bg_color(time % 7 + 1);
		}
	}
}

