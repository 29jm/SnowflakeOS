#include <kernel/multiboot.h>

#include <stdio.h>

void multiboot_print_infos(multiboot_t* boot) {
	if (boot->flags & MULTIBOOT_FRAMEBUFFER) {
		printf("Framebuffer:\n");
		printf("\taddress: 0x%X\n", (uint32_t) boot->framebuffer.address);
		printf("\ttype: %d\n", boot->framebuffer.type);
		printf("\twidth: %d\n", boot->framebuffer.width);
		printf("\theight: %d\n", boot->framebuffer.height);
		printf("\tbpp: %d\n", boot->framebuffer.bpp);
		printf("\tpitch: %d\n", boot->framebuffer.pitch);
	}

	if (boot->flags & MULTIBOOT_FLAG_VBE) {
		// printf("VBE: not implemented\n");
	}

	if (boot->flags & MULTIBOOT_FLAG_MEM) {
		printf("Memory: lower=%dkB upper=%dMB\n", boot->mem_lower, boot->mem_upper/1024);
	}

	if (boot->flags & MULTIBOOT_FLAG_CMDLINE) {
		printf("Command line: %s\n", (char*) boot->cmdline);
	}

	if (boot->flags & MULTIBOOT_FLAG_MODS) {
		printf("Modules: count=%d address=%X\n", boot->mods_count, boot->mods_addr);
	}

	if (boot->flags & MULTIBOOT_FLAG_MMAP) {
		printf("BIOS memory map: address=%X  size=%d\n", boot->mmap_addr, boot->mmap_length);
	}
}