#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/term.h>
#include <kernel/fb.h>
#include <kernel/proc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIRECTORY_INDEX(x) ((x) >> 22)
#define TABLE_INDEX(x) (((x) >> 12) & 0x3FF)

static directory_entry_t* current_page_directory;
extern directory_entry_t kernel_directory[1024];

static uint8_t* heap_pointer = (uint8_t*) KERNEL_HEAP_VIRT;

void init_paging() {
	isr_register_handler(14, &paging_fault_handler);

	// Setup the recursive page directory entry
	uintptr_t dir_phys = VIRT_TO_PHYS((uintptr_t) &kernel_directory);
	kernel_directory[1023] = dir_phys | PAGE_PRESENT | PAGE_RW;
	paging_invalidate_page(0xFFC00000);

	// Identity map only the first 2 MiB: in boot.S, we identity mapped the first 4 MiB
	// We take 2 MiB to give room to GRUB modules.
	kernel_directory[0] = 0;
	paging_invalidate_page(0x00000000);
	paging_map_pages(0x00000000, 0x00000000, 512, PAGE_RW);
	current_page_directory = kernel_directory;

	// Map the kernel heap
	uint32_t heap_pages = (KERNEL_HEAP_VIRT_MAX-KERNEL_HEAP_VIRT)/4096;
	uintptr_t heap_phys = pmm_alloc_pages(heap_pages);
	paging_map_pages(0xD0000000, heap_phys, heap_pages, PAGE_RW);

	// Keep the TTY alive
	uint32_t size = sizeof(uint16_t)*TERM_WIDTH*TERM_HEIGHT;
	uint16_t* tty_buff = (uint16_t*)kamalloc(size, 0x1000);
	page_t* p = paging_get_page((uintptr_t)tty_buff, false, 0);
	*p = TERM_MEMORY | PAGE_PRESENT | PAGE_RW;
	term_set_buffer(tty_buff);

	// Keep the framebuffer alive
	size = fb_get_size();
	uintptr_t fb_buff = (uintptr_t) kamalloc(size, 0x1000);

	for (uint32_t i = 0; i < size/0x1000; i++) {
		p = paging_get_page(fb_buff + 0x1000*i, false, 0);
		*p = (fb_get_address() + 0x1000*i) | PAGE_PRESENT | PAGE_RW;
	}

	fb_set_address(fb_buff);
}

uintptr_t paging_get_kernel_directory() {
	return VIRT_TO_PHYS((uintptr_t) &kernel_directory);
}

/* Given a page-aligned virtual address, returns a pointer to the corresponding
 * page table entry. Usually, this entry is then filled with appropriate page
 * information such as the physical address it points to, whether it is writable
 * etc...
 * If the `create` flag is passed, the corresponding page table is created with
 * the passed flags if needed and this function should never return NULL.
 */
page_t* paging_get_page(uintptr_t virt, bool create, uint32_t flags) {
	if (virt % 0x1000) {
		printf("[VMM] Tried to access a page at an unaligned address!\n");
		abort();
	}

	uint32_t dir_index = DIRECTORY_INDEX(virt);
	uint32_t table_index = TABLE_INDEX(virt);

	directory_entry_t* dir = (directory_entry_t*) 0xFFFFF000;
	page_t* table = (page_t*) (0xFFC00000 + (dir_index << 12));

	if (!(dir[dir_index] & PAGE_PRESENT) && create) {
		page_t* new_table = (page_t*) pmm_alloc_page();
		dir[dir_index] = (uint32_t) new_table
		    | PAGE_PRESENT | PAGE_RW | (flags & PAGE_FLAGS);
		memset((void*) table, 0, 4096);
	}

	if (dir[dir_index] & PAGE_PRESENT) {
		return &table[table_index];
	}

	return NULL;
}

// TODO: refuse 4 MiB pages
void paging_map_page(uintptr_t virt, uintptr_t phys, uint32_t flags) {
	page_t* page = paging_get_page(virt, true, flags);

	if (*page & PAGE_PRESENT) {
		printf("[VMM] Tried to map an already mapped virtual address 0x%X to 0x%X\n",
			virt, phys);
		printf("[VMM] Previous mapping: 0x%X to 0x%X\n", virt, *page & PAGE_FRAME);
		abort();
	}

	*page = phys | PAGE_PRESENT | (flags & PAGE_FLAGS);
	paging_invalidate_page(virt);
}

void paging_unmap_page(uintptr_t virt) {
	page_t* page = paging_get_page(virt, false, 0);

	if (page) {
		*page &= ~(PAGE_PRESENT);
		paging_invalidate_page(virt);
	}
}

void paging_map_pages(uintptr_t virt, uintptr_t phys, uint32_t num, uint32_t flags) {
	for (uint32_t i = 0; i < num; i++) {
		paging_map_page(virt, phys, flags);
		phys += 0x1000;
		virt += 0x1000;
	}
}

void paging_unmap_pages(uintptr_t virt, uint32_t num) {
	for (uint32_t i = 0; i < num; i++) {
		paging_unmap_page(virt);
		virt += 0x1000;
	}
}

void paging_switch_directory(uintptr_t dir_phys) {
	asm volatile("mov %0, %%cr3\n" :: "r"(dir_phys));
	current_page_directory = (directory_entry_t*) PHYS_TO_VIRT(dir_phys);
}

void paging_invalidate_cache() {
	asm (
		"mov %cr3, %eax\n"
		"mov %eax, %cr3\n"
	);
}

void paging_invalidate_page(uintptr_t virt) {
	asm volatile ("invlpg (%0)" :: "b"(virt) : "memory");
}

void paging_fault_handler(registers_t* regs) {
	uint32_t err = regs->err_code;
	uintptr_t cr2 = 0;
	asm volatile("mov %%cr2, %0\n" : "=r"(cr2));

	printf("\x1B[37;44m");
	printf("[VMM] Page Fault caused by instruction at 0x%X from process %d:\n",
	    regs->eip, proc_get_current_pid());
	printf("The page at 0x%X %s present ", cr2, err & 0x01 ? "was" : "wasn't");
	printf("when a process tried to %s it.\n", err & 0x02 ? "write to" : "read from");
	printf("This process was in %s mode.\n", err & 0x04 ? "user" : "kernel");

	page_t* page = paging_get_page(cr2 & PAGE_FRAME, false, 0);

	if (err & 0x01) {
		printf("The page was in %s mode.\n", (*page) & PAGE_USER ? "user" : "kernel");
	}

	if (err & 0x08) {
		printf("The reserved bits were overwritten.\n");
	}

	if (err & 0x10) {
		printf("The fault occured during an instruction fetch.\n");
	}

	printf("\x1B[37;40m");

	if (err & 0x04) { // user process
		proc_exit_current_process();
	}
}

/* Allocates `num` pages of physical memory, mapped starting at `virt`.
 * Beware, pages allocated by this function are not mapped across processes.
 */
void* paging_alloc_pages(uint32_t virt, uintptr_t num) {
	for (uint32_t i = 0; i < num; i++) {
		uintptr_t page = pmm_alloc_page();
		page_t* p = paging_get_page(virt + i*0x1000, true, PAGE_RW | PAGE_USER);
		*p = page | PAGE_PRESENT | PAGE_RW | PAGE_USER;
	}

	return (void*) virt;
}

/* Rerturns the current physical mapping of `virt` if it exists, zero
 * otherwise.
 */
uintptr_t paging_virt_to_phys(uintptr_t virt) {
	page_t* p = paging_get_page(virt & PAGE_FRAME, false, 0);

	if (!p)
		return 0;

	return (((uintptr_t)*p) & PAGE_FRAME) + (virt & 0xFFF);
}

/* Used to allocate memory for use by the kernel.
 * The memory is pre-mapped, which means clones of the kernel page directory
 * (i.e processes) share their kernel memory in kernel mode, for instance
 * during syscalls.
 * There's no corresponding kfree. What the kernel takes, the kernel keeps.
 */
void* kmalloc(uint32_t size) {
	// Accessing basic datatypes at unaligned addresses is apparently undefined
	// behavior. Four-bytes alignement should be enough for most things.
	return kamalloc(size, 4);
}

/* Aligned memory allocator.
 * Returns `size` bytes of memory at an address multiple of `align`.
 */
void* kamalloc(uint32_t size, uint32_t align) {
	uintptr_t next = (((uintptr_t)heap_pointer / align) + 1) * align;

	if (next + size >= KERNEL_HEAP_VIRT_MAX) {
		// printf("[VMM] Kernel out of memory\n");
		// printf("[VMM] Couldn't allocate %d kB: heap at %d\n", size/1024, next);
		return NULL;
	}

	heap_pointer = (uint8_t*)(next + size);

	return (uint8_t*)next;
}