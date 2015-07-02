#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/vga.h>
#include <kernel/tty.h>
#include <kernel/multiboot.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/timer.h>

void kernel_main(multiboot* boot, uint32_t magic) {
	init_terminal();
	init_gdt();
	init_idt();
	init_irq();
	init_timer();

	printf("Welcome to ");
	terminal_setcolor(make_color(COLOR_CYAN, COLOR_DARK_GREY));
	printf("SnowflakeOS");
	terminal_setcolor(make_color(COLOR_LIGHT_GREY, COLOR_DARK_GREY));
	printf(" -1.0 !\n");

	uint32_t tick = timer_get_time();
	while (1) {
		uint32_t ntick = timer_get_time();
		if (ntick != tick) {
			tick = ntick;
			printf("time: %d\n", ntick);
		}
	}
}

