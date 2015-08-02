#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <kernel/tty.h>
#include <kernel/multiboot.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/timer.h>
#include <kernel/pmm.h>
#include <kernel/paging.h>
#include <kernel/keyboard.h>

extern uint32_t KERNEL_BEGIN;
extern uint32_t KERNEL_BEGIN_PHYS;
extern uint32_t KERNEL_END;
extern uint32_t KERNEL_END_PHYS;
extern uint32_t KERNEL_SIZE;

void kernel_main(multiboot* boot, uint32_t magic) {
	init_term();

	printf("Welcome to \x1B[36mSnowflakeOS\x1B[37m -1.0 !\n\n");
	printf("Kernel loaded at 0x%X, ending at 0x%X (%dKiB)\n", &KERNEL_BEGIN_PHYS, &KERNEL_END_PHYS,
		((uint32_t) &KERNEL_SIZE) >> 10);

	assert(magic == MULTIBOOT_EAX_MAGIC);
	assert(boot->flags & MULTIBOOT_FLAG_MMAP);

	init_gdt();
	init_idt();
	init_irq();
	init_timer();
	init_keyboard();
	init_pmm(boot);
	init_paging();

	uint32_t time = 0;
	while (1) {
		uint32_t ntime = (uint32_t) timer_get_time();
		if (ntime > time) {
			time = ntime;
			printf("\x1B[s\x1B[24;76H"); // save & move cursor
			printf("\x1B[K");            // Clear line
			printf("%ds", time);
			printf("\x1B[u");            // Restore cursor
		}
	}
}

