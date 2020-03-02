#include <kernel/mem.h>
#include <kernel/paging.h>
#include <kernel/sys.h>

#include <stdio.h>
#include <stdlib.h>

#define MIN_ALIGN 8

// #define MEM_DEBUG

void mem_print_blocks();

static mem_block_t* bottom = NULL;
static mem_block_t* top = NULL;

uint32_t mem_block_size(mem_block_t* block) {
	return sizeof(mem_block_t) + (block->size & ~1);
}

mem_block_t* mem_get_block(void* pointer) {
	uintptr_t addr = (uintptr_t) pointer;

	return (mem_block_t*) (addr - sizeof(mem_block_t) + 4);
}

/* Returns a new block at the end of the heap, with a `data` member aligned to
 * the specified requirement.
 * Note: may change `top` to prevent memory fragmentation caused by larger
 * alignments.
 */
mem_block_t* mem_new_block(uint32_t size, uint32_t align) {
	const uint32_t header_size = offsetof(mem_block_t, data);

	// We start the heap right where the first allocation works
	if (!top) {
		uintptr_t addr = ALIGN(KERNEL_HEAP_BEGIN+header_size, align) - header_size;
		bottom = (mem_block_t*) addr;
		top = bottom;
		top->size = size | 1;
		top->next = NULL;

		return top;
	}

	// I did the math and we always have next_aligned >= next.
	uintptr_t next = (uintptr_t) top + mem_block_size(top);
	uintptr_t next_aligned = ALIGN(next+header_size, align) - header_size;

	mem_block_t* block = (mem_block_t*) next_aligned;
	block->size = size | 1;
	block->next = NULL;

	// Insert a free block between top and our aligned block, if there's enough
	// space. That block is 8-bytes aligned.
	next = ALIGN(next+header_size, MIN_ALIGN) - header_size;
	if (next_aligned - next > sizeof(mem_block_t) + MIN_ALIGN) {
		mem_block_t* filler = (mem_block_t*) next;
		filler->size = next_aligned - next - sizeof(mem_block_t);

#ifdef MEM_DEBUG
		printf("adding filler block (%p, %d)\n", filler->data, filler->size);
#endif

		filler->next = block;
		top->next = filler;
		top = filler;
	}

	top->next = block;
	top = block;

	return block;
}

bool mem_is_aligned(mem_block_t* block, uint32_t align) {
	uintptr_t addr = (uintptr_t) block->data;

	return addr % align == 0;
}

mem_block_t* mem_find_block(uint32_t size, uint32_t align) {
	if (!bottom) {
		return NULL;
	}

	mem_block_t* block = bottom;

	while (block->size < size || block->size & 1 || !mem_is_aligned(block, align)) {
		block = block->next;

		if (!block) {
			return NULL;
		}
	}

	return block;
}

void mem_print_blocks() {
	mem_block_t* block = bottom;

	while (block) {
		printf("0x%X%s-> ", block->size & ~1, block->size & 1 ? "# " : " ");

		if (block->next && block->next < block) {
			printf("Chaining error: block overlaps with previous one\n");
		}

		block = block->next;
	}

	printf("none\n");
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
	return kamalloc(size, MIN_ALIGN);
}

/* Returns `size` bytes of memory at an address multiple of `align`.
 * The 'a' stands for "aligned".
 */
void* kamalloc(uint32_t size, uint32_t align) {
#ifdef MEM_DEBUG
	printf("\nkamalloc(0x%X, %d)\n", size, align);
#endif
	size = ALIGN(size, 8);

	mem_block_t* block = mem_find_block(size, align);

	if (block) {
		block->size |= 1;
#ifdef MEM_DEBUG
		printf("reusing block (%p, %d)\n", (void*) block->data, block->size);
		printf("-> %p (heap at %p)\n", block->data, top->data);
#endif

		return block->data;
	} else {
		block = mem_new_block(size, align);
	}

	if ((uintptr_t) block->data > KERNEL_HEAP_BEGIN + KERNEL_HEAP_SIZE) {
		printf("[VMM] Kernel ran out of memory!");
		abort();
	}

#ifdef MEM_DEBUG
	mem_print_blocks();
	printf("-> %p\n", block->data, top->data);
#endif

	return block->data;
}

void kfree(void* pointer) {
	mem_block_t* block = mem_get_block(pointer);
	printf("kfree called on block of size %d\n", block->size & ~1);
	block->size &= ~1;
}