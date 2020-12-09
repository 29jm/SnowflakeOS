#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifdef _KERNEL_
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/sys.h>
#endif

#define MIN_ALIGN 4

typedef struct _mem_block_t {
    struct _mem_block_t* next;
    uint32_t size; // We use the last bit as a 'used' flag
    uint8_t data[1];
} mem_block_t;

static mem_block_t* bottom = NULL;
static mem_block_t* top = NULL;
static uint32_t used_memory = 0;

#ifndef _KERNEL_

/* Returns the next multiple of `s` greater than `a`, or `a` if it is a
 * multiple of `s`.
 * Copy of the same function in <kernel/sys.h>
 */
static uint32_t align_to(uint32_t n, uint32_t align) {
    if (n % align == 0) {
        return n;
    }

    return n + (align - n % align);
}

/* Allocates `n` pages of memory located after the program's memory.
 * Returns a pointer to the first allocated page.
 */
static void* sbrk(uint32_t size) {
    uintptr_t addr;

    asm volatile (
        "mov $4, %%eax\n"
        "mov %[size], %%ebx\n"
        "int $0x30\n"
        "mov %%eax, %[addr]\n"
        : [addr] "=r" (addr)
        : [size] "r" (size)
        : "%eax", "%ebx"
    );

    return (void*) addr;
}

#endif

/* Debugging function to print the block list. Only sizes are listed, and a '#'
 * indicates a used block.
 */
void mem_print_blocks() {
    mem_block_t* block = bottom;

    while (block) {
        printf("0x%X%s-> ", block->size & ~1, block->size & 1 ? "# " : " ");

        if (block->next && block->next < block) {
            printf("chaining error: block overlaps with previous one\n");
        }

        block = block->next;
    }

    printf("none\n");
}

/* Returns the size of a block, including the header.
 */
uint32_t mem_block_size(mem_block_t* block) {
    return sizeof(mem_block_t) + (block->size & ~1);
}

/* Returns the block corresponding to `pointer`, given that `pointer` was
 * previously returned by a call to `malloc`.
 */
mem_block_t* mem_get_block(void* pointer) {
    uintptr_t addr = (uintptr_t) pointer;

    return (mem_block_t*) (addr - sizeof(mem_block_t) + 4);
}

/* Appends a new block of the desired size and alignment to the block list.
 * Note: may insert an intermediary block before the one returned to prevent
 * memory fragmentation. Such a block would be aligned to `MIN_ALIGN`.
 */
mem_block_t* mem_new_block(uint32_t size, uint32_t align) {
    const uint32_t header_size = offsetof(mem_block_t, data);

    // I did the math and we always have next_aligned >= next.
    uintptr_t next = (uintptr_t) top + mem_block_size(top);
    uintptr_t next_aligned = align_to(next+header_size, align) - header_size;

    mem_block_t* block = (mem_block_t*) next_aligned;
    block->size = size | 1;
    block->next = NULL;

    // Insert a free block between top and our aligned block, if there's enough
    // space. That block is 8-bytes aligned.
    next = align_to(next+header_size, MIN_ALIGN) - header_size;
    if (next_aligned - next > sizeof(mem_block_t) + MIN_ALIGN) {
        mem_block_t* filler = (mem_block_t*) next;
        filler->size = next_aligned - next - sizeof(mem_block_t);
        top->next = filler;
        top = filler;
    }

    top->next = block;
    top = block;

    return block;
}

/* Returns whether the memory pointed to by `block` is a multiple of `align`.
 */
bool mem_is_aligned(mem_block_t* block, uint32_t align) {
    uintptr_t addr = (uintptr_t) block->data;

    return addr % align == 0;
}

/* Searches the block list for a free block of at least `size` and with the
 * proper alignment. Returns the first corresponding block if any, NULL
 * otherwise.
 */
mem_block_t* mem_find_block(uint32_t size, uint32_t align) {
    if (!bottom) {
        return NULL;
    }

    mem_block_t* block = bottom;

    while ((block->size & ~1) < size || block->size & 1 || !mem_is_aligned(block, align)) {
        block = block->next;

        if (!block) {
            return NULL;
        }
    }

    return block;
}

/* Returns a pointer to a memory area of at least `size` bytes.
 * Note: in the kernel, this function is renamed to `kmalloc`.
 */
void* malloc(size_t size) {
    // Accessing basic datatypes at unaligned addresses is apparently undefined
    // behavior. Four-bytes alignement should be enough for most things.
    return aligned_alloc(MIN_ALIGN, size);
}

void* calloc(size_t nmemb, size_t size) {
    void* ptr = malloc(nmemb * size);

    return memset(ptr, 0, nmemb * size);
}

void* zalloc(size_t size) {
    return calloc(1, size);
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }

    if (!size) {
        free(ptr);
        return NULL;
    }

    void* new = malloc(size);
    memcpy(new, ptr, mem_get_block(ptr)->size & ~1);
    free(ptr);

    return new;
}

/* Frees a pointer previously returned by `malloc`.
 * Note: in the kernel, this function is renamed to `kfree`.
 */
void free(void* pointer) {
    if (!pointer) {
        return;
    }

    mem_block_t* block = mem_get_block(pointer);
    block->size &= ~1;
    used_memory -= block->size;
}

/* Returns `size` bytes of memory at an address multiple of `align`.
 */
void* aligned_alloc(size_t align, size_t size) {
    const uint32_t header_size = offsetof(mem_block_t, data);
    size = align_to(size, 8);

    // If this is the first allocation, setup the block list:
    // it starts with an empty, used block, in order to avoid edge cases.
    if (!top) {
#ifdef _KERNEL_
        uintptr_t addr = KERNEL_HEAP_BEGIN;
        uintptr_t heap_phys = pmm_alloc_pages(KERNEL_HEAP_SIZE/0x1000);
        paging_map_pages(addr, heap_phys, KERNEL_HEAP_SIZE/0x1000, PAGE_RW);
#else
        uintptr_t addr = (uintptr_t) sbrk(header_size);
#endif
        bottom = (mem_block_t*) addr;
        top = bottom;
        top->size = 1; // That means used, of size 0
        top->next = NULL;
    }

    mem_block_t* block = mem_find_block(size, align);

    if (block) {
        used_memory += block->size;
        block->size |= 1;

        return block->data;
    } else {
        // We'll have to allocate a new block, so we check if we haven't
        // exceeded the memory we can distribute.
        uintptr_t end = (uintptr_t) top + mem_block_size(top) + header_size;
        end = align_to(end, align) + size;
#ifdef _KERNEL_
        // The kernel can't allocate more
        if (end > KERNEL_HEAP_BEGIN + KERNEL_HEAP_SIZE) {
            printke("kernel ran out of memory!");
            abort();
        }
#else
        // But userspace can ask the kernel for more
        uintptr_t brk = (uintptr_t) sbrk(0);
        if (end > brk) {
            if (sbrk(end - brk) == (void*) -1) {
                printf("[mem] Allocation failure\n");

                return NULL;
            }
        }
#endif

        block = mem_new_block(size, align);
    }

    used_memory += size;

    return block->data;
}

#ifdef _KERNEL_
/* Alias for `aligned_alloc`.
 * It's a naming habit, don't mind it.
 */
void* kamalloc(uint32_t size, uint32_t align) {
    return aligned_alloc(align, size);
}

/* Returns the memory allocated on the heap by the kernel, in bytes.
 */
uint32_t memory_usage() {
    return used_memory;
}
#endif