#include <kernel/term.h>
#include <kernel/fb.h>
#include <kernel/wm.h>
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
#include <kernel/sys.h>
#include <kernel/fs.h>
#include <kernel/ext2.h>

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern uint32_t KERNEL_BEGIN_PHYS;
extern uint32_t KERNEL_END_PHYS;
extern uint32_t KERNEL_SIZE;

void kernel_main(multiboot_t* boot, uint32_t magic) {
	init_serial();
	init_pmm(boot);
	init_paging();
	init_term();

	if (magic != MULTIBOOT_EAX_MAGIC) {
		printf("The multiboot magic header is wrong: proceeding anyway\n");
	}

	printf("SnowflakeOS 0.5 - Challenge Edition\n");
	printf("Kernel is %d KiB large\n", ((uint32_t) &KERNEL_SIZE) >> 10);

	init_fb(boot->framebuffer);
	// mod_t* m = (mod_t*) boot->mods_addr;
	// for (uint32_t i = 0; i < boot->mods_count; i++) {
	// 	mod_t mod = m[i];
	// 	char* name = (char*) mod.string;
	// 	printf("[kernel] loading %s: %p to %p\n", name, 0xC0000000 + mod.mod_start, 0xC0000000 + mod.mod_end);

	// 	if (!strcmp(name, "background")) {
	// 		printf("[kernel] beginning of \"background\" module at %p\n", mod.mod_start);
	// 		for (uint32_t i = 0; i < 32; i++) {
	// 			printf("%x ", ((uint8_t*) 0xC0000000 + mod.mod_start)[i]);
	// 		}
	// 	}
	// }
	init_gdt();
	init_idt();
	init_isr();
	init_irq();
	init_syscall();

	init_ps2();
	init_timer();
	init_wm();
	init_fs();

	// Load GRUB modules as programs
	mod_t* modules = (mod_t*) boot->mods_addr;

	for (uint32_t i = 0; i < boot->mods_count; i++) {
		mod_t mod = modules[i];
		uint32_t size = mod.mod_end - mod.mod_start;
		char* name = (char*) mod.string;

		uint8_t* code = (uint8_t*) kmalloc(size);
		memcpy((void*) code, (void*) 0xC0000000 + mod.mod_start, size);

		if (!strcmp(name, "disk")) {
			init_ext2(code, size);
			continue;
		} else if (!strcmp(name, "terminal")) {
			proc_run_code(code, size);
		}

		proc_register_program(name, code, size);
	}

	proc_print_processes();

	init_proc();
}
