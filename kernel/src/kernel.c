#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
	init_gdt();
	init_idt();
	init_irq();
	init_timer();
	init_pmm(boot->mem_lower + boot->mem_upper);
	init_keyboard();

	printf("Welcome to \x1B[36mSnowflakeOS\x1B[37m -1.0 !\n\n");

	printf("Kernel loaded at 0x%X, ending at 0x%X (%dKiB)\n", &KERNEL_BEGIN_PHYS, &KERNEL_END_PHYS,
		((uint32_t) &KERNEL_SIZE) >> 10);

	if (magic != MULTIBOOT_EAX_MAGIC) {
		printf("Not a multiboot compliant bootloader... Let's try anyway\n");
	}

	if (boot->flags & MULTIBOOT_FLAG_MMAP) {
		// I didn't even know these worked in protected mode
		// Looking at the assembly, I think they just take up more space,
		// but they're still processed by 32-bits registers
		uint64_t available = 0;
		uint64_t unavailable = 0;
		mmap_t* mmap = (mmap_t*) PHYS_TO_VIRT(boot->mmap_addr);

		while ((uintptr_t) mmap < (PHYS_TO_VIRT(boot->mmap_addr)+boot->mmap_length)) {
			if (mmap->type == 1) {
				pmm_init_region((uintptr_t) mmap->addr, mmap->length);
				available += mmap->length;
			}
			else {
				pmm_deinit_region((uintptr_t) mmap->addr, mmap->length);
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

	// Protect low memory, our glorious kernel and the PMM itself
	pmm_deinit_region((uintptr_t) 0, (uint32_t) &KERNEL_END_PHYS + pmm_get_map_size());

	/* init_paging(); */
	printf("Attempting to page fault..\n");
	int* crash = (int*) 0xCAFEBABE;
	printf("crash: %d\n", *crash);

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

