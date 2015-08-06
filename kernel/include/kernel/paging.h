#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>

#include <kernel/isr.h>

// Page directories are arrays of directory entries
// Page tables are arrays of table entries (TODO: rename page_t)
typedef uint32_t directory_entry_t;
typedef uint32_t page_t;

void init_paging();
page_t* paging_get_page(uintptr_t virt, bool create);
void paging_map_page(uintptr_t virt, uintptr_t phys, uint32_t flags);
void paging_unmap_page(uintptr_t virt);
void paging_map_pages(uintptr_t phys, uintptr_t virt, uint32_t num, uint32_t flags);
void paging_unmap_pages(uintptr_t virt, uint32_t num);
void paging_switch_directory(uintptr_t dir_phys);
void paging_invalidate_cache();
void paging_invalidate_page(uintptr_t virt);
void paging_fault_handler(registers_t* regs);
void* paging_alloc_pages(uint32_t num, uint32_t flags);
int paging_free_pages(uintptr_t virt, uint32_t num);

extern void* kmalloc(size_t size);
extern void kfree(void* virt);
extern void* krealloc(void* virt, size_t size);
extern void* kcalloc(size_t num, size_t size);

#define KERNEL_BASE_VIRT 0xC0000000
#define KERNEL_HEAP_VIRT 0xD0000000

#define PHYS_TO_VIRT(addr) ((addr) + KERNEL_BASE_VIRT)
#define VIRT_TO_PHYS(addr) ((addr) - KERNEL_BASE_VIRT)

#define PAGE_PRESENT 1
#define PAGE_RW      2
#define PAGE_USER    4

#define PAGE_FRAME   0xFFFFF000
#define PAGE_FLAGS   0x00000FFF

#endif
