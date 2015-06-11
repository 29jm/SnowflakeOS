#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

#define MULTIBOOT_MAGIC        0x2BADB002
#define MULTIBOOT_EAX_MAGIC    0x2BADB002
#define MULTIBOOT_FLAG_MEM     (1 << 0)
#define MULTIBOOT_FLAG_DEVICE  (1 << 1)
#define MULTIBOOT_FLAG_CMDLINE (1 << 2)
#define MULTIBOOT_FLAG_MODS    (1 << 3)
#define MULTIBOOT_FLAG_AOUT    (1 << 4)
#define MULTIBOOT_FLAG_ELF     (1 << 5)
#define MULTIBOOT_FLAG_MMAP    (1 << 6)
#define MULTIBOOT_FLAG_CONFIG  (1 << 7)
#define MULTIBOOT_FLAG_LOADER  (1 << 8)
#define MULTIBOOT_FLAG_APM     (1 << 9)
#define MULTIBOOT_FLAG_VBE     (1 << 10)

typedef struct {

} __attribute__ ((packed)) vbe_info_t;

typedef struct {
	uint8_t drive_number;
	uint8_t part1;
	uint8_t part2;
	uint8_t part3;
} __attribute__ ((packed)) boot_device_t;

typedef struct {
	uintptr_t		flags;
	uintptr_t		mem_lower;
	uintptr_t		mem_upper;
	boot_device_t*	boot_device;
	char*			cmdline;
	uintptr_t		mods_count;
	uintptr_t		mods_addr;
	uintptr_t		num;
	uintptr_t		size;
	uintptr_t		addr;
	uintptr_t		shndx;
	uintptr_t		mmap_length;
	uintptr_t		mmap_addr;
	uintptr_t		drives_length;
	uintptr_t		drives_addr;
	uintptr_t		config_table;
	uintptr_t		boot_loader_name;
	uintptr_t		apm_table;
	uintptr_t		vbe_control_info;
	uintptr_t		vbe_mode_info;
	uintptr_t		vbe_mode;
	uintptr_t		vbe_interface_seg;
	uintptr_t		vbe_interface_off;
	uintptr_t		vbe_interface_len;
} __attribute__ ((packed)) multiboot;

#endif
