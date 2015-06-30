#include <stddef.h>
#include <stdbool.h>
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
	init_terminal();
	init_gdt();
	init_idt();
	init_irq();
	init_timer();

	uint32_t tick = timer_get_tick();
	while (1) {
		uint32_t ntick = timer_get_tick();
		if (ntick != tick) {
			tick = ntick;
			printf("tick: %d\n", ntick);
		}
	}
}

