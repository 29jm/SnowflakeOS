#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <string.h>
#include <kernel/isr.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DIRECTORY_INDEX(x) ((x) >> 22)
#define TABLE_INDEX(x) (((x) >> 12) & 0x3FF)

static directory_entry_t* current_page_directory;
extern directory_entry_t kernel_directory[1024];

void init_paging() {
	isr_register_handler(14, &paging_fault_handler);

	// Setup the recursive page directory entry
	// TODO: do that in boot.S
	uintptr_t dir_phys = VIRT_TO_PHYS((uintptr_t) &kernel_directory);
	kernel_directory[1023] = dir_phys | PAGE_PRESENT | PAGE_RW;
	paging_invalidate_page(0xFFC00000);
}

// TODO: refuse 4 MiB pages
void paging_map_page(uintptr_t virt, uintptr_t phys, uint32_t flags) {
	page_t* page = paging_get_page(virt, true);

	if (*page & PAGE_PRESENT) {
		printf("[VMM] Tried to map an already mapped virtual address!\n");
		abort();
	}
 
    *page = phys | PAGE_PRESENT | (flags & 0xFFF);
	paging_invalidate_page(virt);
}

page_t* paging_get_page(uintptr_t virt, bool create) {
	if (virt % 0x1000) {
		printf("[VMM] Tried to access a page at an unaligned address!\n");
		abort();
	}

	uint32_t dir_index = DIRECTORY_INDEX(virt);
	uint32_t table_index = TABLE_INDEX(virt);

	directory_entry_t* dir = (directory_entry_t*) 0xFFFFF000;
	page_t* table = ((uint32_t*) 0xFFC00000) + (0x400 * dir_index);

	if (!(dir[dir_index] & PAGE_PRESENT && create)) {
		page_t* new_table = (page_t*) pmm_alloc_page();
		dir[dir_index] = (uint32_t) new_table | PAGE_PRESENT | PAGE_RW;
		memset((void*) table, 0, 0x1000);
	}
	else {
		return 0;
	}

	return &table[table_index];
}

void paging_unmap_page(uintptr_t virt) {
	page_t* page = paging_get_page(virt, false);

	if (page) {
		*page &= ~(PAGE_PRESENT);
		paging_invalidate_page(virt);
	}
}

void paging_map_memory(uintptr_t phys, uintptr_t virt, uint32_t size) {
	// size / 0x1000 = number of pages to map
	for (uint32_t i = 0; i < size / 0x1000; i++) {
		paging_map_page(phys, virt, PAGE_RW);
		phys += 0x1000;
		virt += 0x1000;
	}
}

void paging_switch_directory(uintptr_t dir_phys) {
	asm volatile("mov %0, %%cr3\n" :: "r"(dir_phys));
	current_page_directory = (directory_entry_t*) PHYS_TO_VIRT(dir_phys);
}

void paging_invalidate_cache() {
	asm volatile(
		"mov %cr3, %eax\n"
		"mov %eax, %cr3\n");
}

void paging_invalidate_page(uintptr_t virt) {
    asm volatile ("invlpg (%0)" :: "b"(virt) : "memory");
}

void paging_fault_handler(registers_t* regs) {
	uint32_t err = regs->err_code;

	printf("The page requested %s present ", err & 0x01 ? "was" : "wasn't");
	printf("when a process tried to %s it.\n", err & 0x02 ? "write to" : "read from");
	printf("This process was in %s mode.\n", err & 0x04 ? "user" : "kernel");
	printf("The reserved bits %s overwritten.\n", err & 0x08 ? "were" : "weren't");
	printf("The fault %s during an instruction fetch.\n", err & 0x10 ? "occured" : "didn't occur");
}
