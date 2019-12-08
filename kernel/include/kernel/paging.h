#ifndef PAGING_H
#define PAGING_H

#include <kernel/isr.h>

#include <stdint.h>
#include <stdbool.h>

// Page directories are arrays of directory entries
// Page tables are arrays of table entries
typedef uint32_t directory_entry_t;
typedef uint32_t page_t;

void init_paging();
uintptr_t paging_get_kernel_directory();
page_t* paging_get_page(uintptr_t virt, bool create, uint32_t flags);
void paging_map_page(uintptr_t virt, uintptr_t phys, uint32_t flags);
void paging_unmap_page(uintptr_t virt);
void paging_map_pages(uintptr_t phys, uintptr_t virt, uint32_t num, uint32_t flags);
void paging_unmap_pages(uintptr_t virt, uint32_t num);
void paging_switch_directory(uintptr_t dir_phys);
void paging_invalidate_cache();
void paging_invalidate_page(uintptr_t virt);
void paging_fault_handler(registers_t* regs);
void* paging_alloc_pages(uint32_t virt, uint32_t num);
void paging_free_pages(uintptr_t virt, uint32_t num);
uintptr_t paging_virt_to_phys(uintptr_t virt);

void* kmalloc(uint32_t size);
void* kamalloc(uint32_t size, uint32_t align);

/* We assume here that the kernel code data and stack as setup by our linker
 * script ends before addresses 0xD0000000, which is a fair guess, as long as
 * our kernel occupies less than 255 MiB of memory.
 * We could also use the `KERNEl_END_PHYS` linker symbol to avoid making these
 * assumptions, and probably to gain address space, but having clear separations
 * has advantages in debugging.
 */
#define KERNEL_BASE_VIRT 0xC0000000

// Get ourselves ~4 MiB of heap space
#define KERNEL_HEAP_VIRT 0xD0000000
#define KERNEL_HEAP_VIRT_MAX 0xD0400000

#define PHYS_TO_VIRT(addr) ((addr) + KERNEL_BASE_VIRT)
#define VIRT_TO_PHYS(addr) ((addr) - KERNEL_BASE_VIRT)

#define PAGE_PRESENT 1
#define PAGE_RW      2
#define PAGE_USER    4
#define PAGE_LARGE   128

#define PAGE_FRAME   0xFFFFF000
#define PAGE_FLAGS   0x00000FFF

#endif
