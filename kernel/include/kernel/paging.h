#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <kernel/isr.h>

typedef struct {
	uint32_t present:1;
	uint32_t rw:1;
	uint32_t user:1;
	uint32_t writethrough:1;
	uint32_t disable_cache:1;
	uint32_t accessed:1;
	uint32_t zero:1;
	uint32_t large_pages:1;
	uint32_t unused:1;
	uint32_t available:3;
	uint32_t frame:20;
} __attribute__((packed)) directory_entry_t;

typedef struct {
	uint32_t present:1;
	uint32_t rw:1;
	uint32_t user:1;
	uint32_t writethrough:1;
	uint32_t disable_cache:1;
	uint32_t accessed:1;
	uint32_t dirty:1;
	uint32_t zero:1;
	uint32_t global:1;
	uint32_t available:3;
	uint32_t frame:20;
} __attribute__((packed)) page_t;

typedef struct {
	page_t entries[1024];
} page_table_t;

typedef struct {
	directory_entry_t entries[1024];
} page_directory_t;


void init_paging();

#define KERNEL_BASE_VIRT 0xC0000000
#define PHYS_TO_VIRT(addr) ((addr) + KERNEL_BASE_VIRT)
#define VIRT_TO_PHYS(addr) ((addr) - KERNEL_BASE_VIRT)

#endif
