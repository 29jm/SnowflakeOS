#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

#define MULTIBOOT_MAGIC        0x1BADB002
#define MULTIBOOT_EAX_MAGIC    0x2BADB002
#define MULTIBOOT_FLAG_MEM     (1 << 0)
#define MULTIBOOT_FLAG_DEVICE  (1 << 1)
#define MULTIBOOT_FLAG_CMDLINE (1 << 2)
#define MULTIBOOT_FLAG_MODS    (1 << 3)
#define MULTIBOOT_FLAG_AOUT    (1 << 4)
#define MULTIBOOT_FLAG_ELF     (1 << 5)
#define MULTIBOOT_FLAG_MMAP    (1 << 6)
#define MULTIBOOT_FLAG_DRIVES  (1 << 7)
#define MULTIBOOT_FLAG_CONFIG  (1 << 8)
#define MULTIBOOT_FLAG_LOADER  (1 << 9)
#define MULTIBOOT_FLAG_APM     (1 << 10)
#define MULTIBOOT_FLAG_VBE     (1 << 11)
#define MULTIBOOT_FRAMEBUFFER  (1 << 12)

typedef struct {
	uintptr_t mod_start;
	uintptr_t mod_end;
	uintptr_t string;
	uint32_t reserved;
} mod_t;

typedef struct {
	uint32_t size;
	uint64_t addr;
	uint64_t length;
	uint32_t type;
} __attribute__ ((packed)) mmap_t;

typedef struct {
	uint64_t address;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint8_t bpp;
	uint8_t type;
	uint8_t red_position;
	uint8_t red_mask_size;
	uint8_t green_position;
	uint8_t green_mask_size;
	uint8_t blue_position;
	uint8_t blue_mask_size;
} __attribute__ ((packed)) fb_info_t;

typedef struct {
	uintptr_t flags;
	uintptr_t mem_lower;
	uintptr_t mem_upper;
	uintptr_t boot_device;
	uintptr_t cmdline;
	uintptr_t mods_count;
	uintptr_t mods_addr;
	uintptr_t num;
	uintptr_t size;
	uintptr_t addr;
	uintptr_t shndx;
	uintptr_t mmap_length;
	uintptr_t mmap_addr;
	uintptr_t drives_length;
	uintptr_t drives_addr;
	uintptr_t config_table;
	uintptr_t boot_loader_name;
	uintptr_t apm_table;
	/* VBE */
	uintptr_t vbe_control_info;
	uintptr_t vbe_mode_info;
	uint16_t vbe_mode;
	uint16_t vbe_interface_seg;
	uint16_t vbe_interface_off;
	uint16_t vbe_interface_len;
	/* framebuffer */
	fb_info_t framebuffer;
} __attribute__ ((packed)) multiboot_t;

void multiboot_print_infos(multiboot_t* boot);

#endif