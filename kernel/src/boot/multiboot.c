#include <stdio.h>

#include <kernel/multiboot.h>

void dump_multiboot_infos(multiboot* boot) {
	printf("Flags:\n");
	printf("\tFLAG_MEM: %s\n", (boot->flags & MULTIBOOT_FLAG_MEM ? "Set" : "Not set"));
	printf("\tFLAG_DEVICE: %s\n", (boot->flags & MULTIBOOT_FLAG_DEVICE ? "Set" : "Not set"));
	printf("\tFLAG_CMDLINE: %s\n", (boot->flags & MULTIBOOT_FLAG_CMDLINE ? "Set" : "Not set"));
	printf("\tFLAG_MODS: %s\n", (boot->flags & MULTIBOOT_FLAG_MODS ? "Set" : "Not set"));
	printf("\tFLAG_AOUT: %s\n", (boot->flags & MULTIBOOT_FLAG_AOUT ? "Set" : "Not set"));
	printf("\tFLAG_ELF: %s\n", (boot->flags & MULTIBOOT_FLAG_ELF ? "Set" : "Not set"));
	printf("\tFLAG_MMAP: %s\n", (boot->flags & MULTIBOOT_FLAG_MMAP ? "Set" : "Not set"));
	printf("\tFLAG_CONFIG: %s\n", (boot->flags & MULTIBOOT_FLAG_CONFIG ? "Set" : "Not set"));
	printf("\tFLAG_LOADER: %s\n", (boot->flags & MULTIBOOT_FLAG_LOADER ? "Set" : "Not set"));
	printf("\tFLAG_APM: %s\n", (boot->flags & MULTIBOOT_FLAG_APM ? "Set" : "Not set"));
	printf("\tFLAG_VBE: %s\n", (boot->flags & MULTIBOOT_FLAG_VBE ? "Set" : "Not set"));

	if (boot->flags & MULTIBOOT_FLAG_MEM) {
		printf("Memory: lower=%dkB upper=%dMB\n", boot->mem_lower, boot->mem_upper/1024);
	}

	if (boot->flags & MULTIBOOT_FLAG_DEVICE) {
		boot_device_t* device = (boot_device_t*) boot->boot_device;
		printf("Boot device: number=%X part1=%X part2=%X part3=%X\n", device->drive_number, device->part1,
				device->part2, device->part3);
	}

	if (boot->flags & MULTIBOOT_FLAG_CMDLINE) {
		printf("Command line: %s\n", (char*) boot->cmdline);
	}

	if (boot->flags & MULTIBOOT_FLAG_MODS) {
		printf("Modules: count=%d address=%X\n", boot->mods_count, boot->mods_addr);
	}

	if (boot->flags & MULTIBOOT_FLAG_AOUT) {
		printf("Symbol table: not implemented\n");
	}

	if (boot->flags & MULTIBOOT_FLAG_ELF) {
		printf("ELF infos: number=%d size=%d\n", boot->num, boot->size);
	}

	if (boot->flags & MULTIBOOT_FLAG_MMAP) {
		printf("BIOS memory map: address=%X  size=%d\n", boot->mmap_addr, boot->mmap_length);
	}

	if (boot->flags & MULTIBOOT_FLAG_CONFIG) {
		printf("Drives: address=%X length=%d\n", boot->drives_addr, boot->drives_length);
	}

	if (boot->flags & MULTIBOOT_FLAG_LOADER) {
		printf("Boot loader name: %s\n", (char*) boot->boot_loader_name);
	}

	if (boot->flags & MULTIBOOT_FLAG_APM) {
		printf("APM: address=%X\n", boot->apm_table);
	}

	if (boot->flags & MULTIBOOT_FLAG_VBE) {
		printf("VBE: not implemented\n");
	}
}
