#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/vga.h>
#include <kernel/multiboot.h>

void kernel_main(multiboot* boot, uintptr_t esp, uint32_t magic) {
	terminal_init();

	printf("################################################################################\n");
	printf("######                                                                    ######\n");
	printf("####                         Multiboot Header                               ####\n");
	printf("###                                                                          ###\n");
	printf("##                                                                            ##\n");
	printf("#                                                                              #\n");
	printf("\tMagic number: %s\n", magic == 0x1BADB002 ? "Valid" : "Invalid");
	printf("\tCommand line: %s\n", boot->cmdline);
}

