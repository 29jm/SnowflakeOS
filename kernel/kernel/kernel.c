#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/vga.h>
#include <kernel/multiboot.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>

void kernel_main(multiboot* boot, uint32_t magic) {
	terminal_init();

	init_gdt();
	// init_idt();

	bool valid_magic = (magic == MULTIBOOT_MAGIC);

	printf("################################################################################\n");
	printf("######                                                                    ######\n");
	printf("####                         Multiboot Header                               ####\n");
	printf("###                                                                          ###\n");
	printf("##                                                                            ##\n");
	printf("#                                                                              #\n");
	printf("\tMagic number: %X (%s)\n", magic, valid_magic ? "Valid" : "Invalid");
	if (boot->flags & MULTIBOOT_FLAG_CMDLINE)
		printf("\tCommand line: %s\n", boot->cmdline);
	if (boot->flags & MULTIBOOT_FLAG_DEVICE) {
		printf("\tBoot device infos: drive number: %d\tpart1: %d\t part2: %d", boot->boot_device->drive_number,
				boot->boot_device->part1, boot->boot_device->part2);
	}
	printf("\n\nWill try interrupts...\n");


	/* asm volatile("int $0x3"); */
	/* asm volatile("int $0x4"); */
}

