#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/vga.h>
#include <kernel/multiboot.h>

void kernel_main(multiboot* boot, uintptr_t esp, uint32_t magic) {
	terminal_init();

	bool valid_magic = (magic == MULTIBOOT_MAGIC);

	printf("################################################################################\n");
	printf("######                                                                    ######\n");
	printf("####                         Multiboot Header                               ####\n");
	printf("###                                                                          ###\n");
	printf("##                                                                            ##\n");
	printf("#                                                                              #\n");
	printf("\tMagic number: %x (%s)\n", magic, valid_magic ? "Valid" : "Invalid");
	printf("\tCommand line: %s\n", boot->cmdline);
}

