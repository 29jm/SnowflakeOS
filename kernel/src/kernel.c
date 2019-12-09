#include <kernel/term.h>
#include <kernel/fb.h>
#include <kernel/serial.h>
#include <kernel/multiboot.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/timer.h>
#include <kernel/pmm.h>
#include <kernel/paging.h>
#include <kernel/ps2.h>
#include <kernel/syscall.h>
#include <kernel/proc.h>

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern uint32_t KERNEL_BEGIN_PHYS;
extern uint32_t KERNEL_END_PHYS;
extern uint32_t KERNEL_SIZE;

void kernel_main(multiboot_t* boot, uint32_t magic) {
	init_term();
	init_fb(boot->framebuffer);

	printf("\x1B[36;1mSnowflakeOS\x1B[37m 0.2 - Challenge Edition\n\n");
	printf("Kernel loaded at 0x%X, ending at 0x%X (%dKiB)\n",
		&KERNEL_BEGIN_PHYS, &KERNEL_END_PHYS, ((uint32_t) &KERNEL_SIZE) >> 10);

	assert(magic == MULTIBOOT_EAX_MAGIC);
	assert(boot->flags & MULTIBOOT_FLAG_MMAP);

	init_serial();
	multiboot_print_infos(boot);
	init_gdt();
	init_idt();
	init_isr();
	init_irq();
	init_ps2();
	init_timer();
	init_pmm(boot);
	init_paging();
	init_syscall();

	mod_t* modules = (mod_t*) boot->mods_addr;

	for (uint32_t i = 0; i < boot->mods_count; i++) {
		mod_t mod = modules[i];
		uint32_t len = mod.mod_end - mod.mod_start;
		uint8_t* code = (uint8_t*) kmalloc(len);
		memcpy((void*) code, (void*) mod.mod_start, len);
		proc_run_code(code, len);
	}

	proc_print_processes();

	init_proc();
}
