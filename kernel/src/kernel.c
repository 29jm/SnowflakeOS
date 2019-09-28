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
#include <kernel/syscall.h>
#include <kernel/proc.h>

extern uint32_t KERNEL_BEGIN_PHYS;
extern uint32_t KERNEL_END_PHYS;
extern uint32_t KERNEL_SIZE;

void kernel_main(multiboot* boot, uint32_t magic) {
	init_term();

	printf("\x1B[36mSnowflakeOS\x1B[37m 0.1 - Challenge Edition\n\n");
	printf("Kernel loaded at 0x%X, ending at 0x%X (%dKiB)\n",
		&KERNEL_BEGIN_PHYS, &KERNEL_END_PHYS, ((uint32_t) &KERNEL_SIZE) >> 10);

	assert(magic == MULTIBOOT_EAX_MAGIC);
	assert(boot->flags & MULTIBOOT_FLAG_MMAP);

	init_gdt();
	init_idt();
	init_irq();
	init_timer();
	init_keyboard();
	init_pmm(boot);
	init_paging();
	init_syscall();

	uint8_t p1[] = {
		0xb9, 0x2A, 0x00, 0x00, 0x00, // Sets %ecx to 42
		0xb8, 0x00, 0x00, 0x00, 0x00,
		// 0x66, 0x87, 0xDB, // xchgw %bx, %bx
		0xCD, 0x30,
		0xEB, 0xF2
	};

	uint8_t p2[] = {
		0xb9, 0x45, 0x00, 0x00, 0x00, // Sets it to 69
		0xb8, 0x00, 0x00, 0x00, 0x00,
		0xCD, 0x30,
		0xEB, 0xF2
	};

	proc_run_code(p1, sizeof(p1));
	proc_run_code(p2, sizeof(p2));

	proc_print_processes();

	init_proc();

	uint32_t time = 0;
	while (1) {
		uint32_t ntime = (uint32_t) timer_get_time();
		if (ntime > time) {
			time = ntime;
			printf("\x1B[s\x1B[24;75H"); // save & move cursor
			printf("\x1B[K");            // Clear line
			printf("%ds", time);
			printf("\x1B[u");            // Restore cursor
		}
	}
}
