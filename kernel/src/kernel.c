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

	printf("Welcome to \x1B[36mSnowflakeOS\x1B[37m -1.0 !\n\n");

	if (boot->flags & MULTIBOOT_FLAG_MMAP) {
		// I didn't even know these worked in protected mode
		// Looking at the assembly, I think they just take up more space,
		// but they're still processed by 32-bits registers
		uint64_t available = 0;
		uint64_t unavailable = 0;
		mmap_t* mmap = (mmap_t*) boot->mmap_addr;

		while ((uintptr_t) mmap < (boot->mmap_addr+boot->mmap_length)) {
			if (mmap->type == 1) {
				available += mmap->length;
			}
			else {
				unavailable += mmap->length;
			}

			// Casts needed to get around pointer increment magic
			mmap = (mmap_t*) ((uintptr_t) mmap + mmap->size + sizeof(uintptr_t));
		}

		printf("Total available memory: \x1B[32m%dMiB\x1B[37m\n", available >> 20);
		printf("Total unavailable memory: \x1B[32m%dKiB\x1B[37m\n", unavailable >> 10);
	}
	else {
		printf("ERROR: No memory map given, catching fire...\n");
		abort();
	}

	uint32_t time = 0;
	while (1) {
		uint32_t ntime = timer_get_time();
		if (ntime != time) {
			time = ntime;
			printf("%d ", time);
		}
	}
}

