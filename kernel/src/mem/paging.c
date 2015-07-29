#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <string.h>
#include <kernel/isr.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)

static page_directory_t* current_page_directory;
static page_directory_t* kernel_directory;
static page_directory_t page_directory_pool[1024];

page_t* get_page(page_directory_t* dir, uintptr_t virt, bool create) {
	uint32_t dir_index = PAGE_DIRECTORY_INDEX(virt);
	uint32_t table_index = PAGE_TABLE_INDEX(virt);
	directory_entry_t* dir_entry = &dir->entries[dir_index];

	if (!dir_entry->present && create) {
		dir_entry->frame = (uint32_t) pmm_alloc_page();
		dir_entry->present = 1;
		dir_entry->rw = 1;

		memset((void*) dir_entry->frame, 0, sizeof(page_table_t));
	} 

	if (dir_entry->present) {
		page_table_t* table = (page_table_t*) dir_entry->frame;
		page_t* page = &table->entries[table_index];

		return page;
	}

	return 0;
}

page_t* configure_page(page_t* page, bool kernel, bool rw) {
	// These are single bits, can't assign directly
	page->user = kernel ? 0 : 1;
	page->rw = rw ? 1 : 0;

	return page;
}

page_t* map_page(page_directory_t* dir, uintptr_t phys, uintptr_t virt) {
	if (phys % 0x1000 || virt % 0x1000) {
		printf("Tried to map unaligned addresses!");
		abort();
	}

	page_t* page = get_page(dir, virt, true);
	page->frame = phys;

	return page;
}

void free_page(page_t* page) {
	if (page->frame) {
		pmm_free_page(page->frame);
	}

	// For safety
	page->present = 0;
}

void map_memory(page_directory_t* dir, uintptr_t phys, uintptr_t virt, uint32_t size) {
	// size / 0x1000 = number of pages to map
	for (uint32_t i = 0; i < size / 0x1000; i++) {
		configure_page(map_page(dir, phys, virt), true, true);
		phys += 0x1000;
		virt += 0x1000;
	}
}

void enable_paging() {
	asm volatile(
		"mov %cr0, %eax\n"
		"or $0x80000000, %eax\n"
		"mov %eax, %cr0\n");
}

void switch_directory(page_directory_t* dir) {
	asm volatile("mov %0, %%cr3\n" :: "r"(dir));
	current_page_directory = dir;
}

void invalidate_page(uintptr_t virt) {
	asm volatile(
		"mov %0, %%eax\n"
		"invlpg (%%eax)\n"
		:: "r"(virt) : "%eax");
}

void fault_handler(registers_t* regs) {
	uint32_t err = regs->err_code;

	// abort(); // TODO: remove

	printf("The page requested %s present ", err & 0x01 ? "was" : "wasn't");
	printf("when a process tried to %s it.\n", err & 0x02 ? "write to" : "read from");
	printf("This process was in %s mode.\n", err & 0x04 ? "user" : "kernel");
	printf("The reserved bits %s overwritten.\n", err & 0x08 ? "were" : "weren't");
	printf("The fault %s during an instruction fetch.\n", err & 0x10 ? "occured" : "didn't occur");
}

void init_paging() {
	asm volatile("xchgw %bx, %bx");
	kernel_directory = &page_directory_pool[0];
	memset(kernel_directory, 0, sizeof(page_directory_t));

	map_memory(kernel_directory, 0x00000000, 0xC0000000, 0x800000);

	isr_register_handler(14, fault_handler);

	switch_directory(kernel_directory);
}
