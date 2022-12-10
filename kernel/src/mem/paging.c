#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/proc.h>
#include <kernel/stacktrace.h>
#include <kernel/sys.h>
#include <kernel/term.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIRECTORY_INDEX(x) ((x) >> 22)
#define TABLE_INDEX(x) (((x) >> 12) & 0x3FF)

static directory_entry_t* current_page_directory;

extern directory_entry_t kernel_directory[1024];

void init_paging(mb2_t* boot) {
    isr_register_handler(14, &paging_fault_handler);

    // Setup the recursive page directory entry
    uintptr_t dir_phys = VIRT_TO_PHYS((uintptr_t) &kernel_directory);
    kernel_directory[1023] = dir_phys | PAGE_PRESENT | PAGE_RW;
    paging_invalidate_page(0xFFC00000);

    // Replace the initial identity mapping, extending it to cover grub modules
    uint32_t end = max((uintptr_t) boot + boot->total_size, pmm_get_kernel_end());
    uint32_t to_map = divide_up(end, 0x1000);
    memset(kernel_directory, 0, (DIRECTORY_INDEX(KERNEL_BASE_VIRT) - 1) * sizeof(directory_entry_t));
    paging_map_pages(0x00000000, 0x00000000, to_map, PAGE_RW);
    paging_invalidate_page(0x00000000);
    current_page_directory = kernel_directory;
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
        printke("Paging_get_page: unaligned address %p",virt);
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
        printke("tried to map an already mapped virtual address 0x%X to 0x%X",
            virt, phys);
        printke("previous mapping: 0x%X to 0x%X", virt, *page & PAGE_FRAME);
        abort();
    }

    *page = phys | PAGE_PRESENT | (flags & PAGE_FLAGS);
    paging_invalidate_page(virt);
}

void paging_unmap_page(uintptr_t virt) {
    page_t* page = paging_get_page(virt, false, 0);

    if (page) {
        pmm_free_page(*page & PAGE_FRAME);
        *page = 0;
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
    asm volatile("mov %0, %%cr3\n" :: "r" (dir_phys));
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
    if (!regs) {
        printke("weird page fault");
        abort();
    }

    uint32_t err = regs->err_code;
    uint32_t pid = proc_get_current_pid();
    uintptr_t cr2 = 0;
    asm volatile("mov %%cr2, %0\n" : "=r"(cr2));

    printke("page fault caused by instruction at %p from process %d:",
        regs->eip, pid);
    printke("the page at %p %s present ", cr2, err & 0x01 ? "was" : "wasn't");
    printke("when a process tried to %s it", err & 0x02 ? "write to" : "read from");
    printke("this process was in %s mode", err & 0x04 ? "user" : "kernel");

    page_t* page = paging_get_page(cr2 & PAGE_FRAME, false, 0);

    if (page) {
        if (err & 0x01) {
            printke("The page was in %s mode", (*page) & PAGE_USER ? "user" : "kernel");
        }
    }

    if (err & 0x08) {
        printke("The reserved bits were overwritten");
    }

    if (err & 0x10) {
        printke("The fault occured during an instruction fetch");
    }

    if (!(err & 0x04)) {
        stacktrace_print();
    }

    if (pid) {
        proc_exit();
    } else {
        abort();
    }
}

/* Allocates `num` pages of physical memory, mapped starting at `virt`.
 * Note: pages allocated by this function are not mapped across processes.
 */
void* paging_alloc_pages(uint32_t virt, uintptr_t size) {
    for (uint32_t i = 0; i < size; i++) {
        uintptr_t page = pmm_alloc_page();

        if (!page) {
            return NULL;
        }

        page_t* p = paging_get_page(virt + i*0x1000, true, PAGE_RW | PAGE_USER);
        *p = page | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    return (void*) virt;
}

/* Returns the current physical mapping of `virt` if it exists, zero
 * otherwise.
 */
uintptr_t paging_virt_to_phys(uintptr_t virt) {
    page_t* p = paging_get_page(virt & PAGE_FRAME, false, 0);

    if (!p) {
        return 0;
    }

    return (((uintptr_t)*p) & PAGE_FRAME) + (virt & 0xFFF);
}

bool paging_disable_page_cache(void* virt) {
    page_t* page = paging_get_page((uintptr_t) virt, false, 0);
    if (page)
        *page |= PAGE_CACHE_DISABLE;
    else
        return false;

    return true;
}
