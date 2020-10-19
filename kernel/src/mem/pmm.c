#include <kernel/pmm.h>
#include <kernel/paging.h> // PHYS_TO_VIRT
#include <kernel/multiboot.h>
#include <kernel/sys.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define NTHBIT(n) ((uint32_t) 1 << n)

static uint32_t bitmap[1024 * 1024 / 32];
static uint32_t mem_size;
static uint32_t used_blocks;
static uint32_t max_blocks;
static uintptr_t kernel_end;

// Linker-provided symbols. Beware, those don't take into account GRUB's things
extern uint32_t KERNEL_END;
extern uint32_t KERNEL_END_PHYS;

void mmap_set(uint32_t bit);
void mmap_unset(uint32_t bit);
uint32_t mmap_test(uint32_t bit);
uint32_t mmap_find_free();
uint32_t mmap_find_free_frame(uint32_t num);

void init_pmm(multiboot_t* boot) {
	// Compute where the kernel & GRUB modules ends in physical memory
	mod_t* modules = (mod_t*) boot->mods_addr;

	for (uint32_t i = 0; i < boot->mods_count; i++) {
		mod_t mod = modules[i];
		kernel_end = mod.mod_end;
	}

	// In the author's case, modules come after the kernel,
	// but there's no guarantee it'll always be true
	if (kernel_end < (uintptr_t) &KERNEL_END_PHYS) {
		kernel_end = (uintptr_t) &KERNEL_END_PHYS;
	}

	if ((uint32_t) &KERNEL_END > KERNEL_END_MAP) {
		printf("[pmm] the kernel is too large for its initial mapping\n");
		abort();
	}

	// Prepare our block allocation bitmap
	mem_size = boot->mem_lower + boot->mem_upper;
	max_blocks = (mem_size * 1024) / PMM_BLOCK_SIZE; // `mem_size` is in KiB
	used_blocks = max_blocks;
	memset(bitmap, 0xFF, max_blocks/8); // Blocks are taken by default

	// Parse the memory map to mark valid areas as available
	uint64_t available = 0;
	uint64_t unavailable = 0;

	mmap_t* mmap = (mmap_t*) PHYS_TO_VIRT(boot->mmap_addr);

	while ((uintptr_t) mmap < (PHYS_TO_VIRT(boot->mmap_addr)+boot->mmap_length)) {
		if (!mmap->length) {
			continue;
		}

		if (mmap->type == 1) {
			pmm_init_region((uintptr_t) mmap->addr, mmap->length);
			available += mmap->length;
		} else {
			unavailable += mmap->length;
		}

		// Casts needed to get around pointer increment magic
		mmap = (mmap_t*) ((uintptr_t) mmap + mmap->size + sizeof(uintptr_t));
	}

	mem_size = available;

	// Protect low memory, our glorious kernel and the PMM itself
	pmm_deinit_region(0, kernel_end + max_blocks/8);

	printf("[PMM] Memory stats: available: \x1B[32m%dMiB", available >> 20);
	printf("\x1B[37m unavailable: \x1B[32m%dKiB\x1B[37m\n", unavailable >> 10);
	printf("[PMM] Taken by modules: \x1B[32m%dKiB\x1B[37m\n",
		((uintptr_t) bitmap - (uintptr_t) &KERNEL_END) >> 10);
}

/* Returns the number of bytes allocated by the PMM.
 */
uint32_t pmm_used_memory() {
	return used_blocks * PMM_BLOCK_SIZE;
}

/* Returns the number of free bytes the PMM started with.
 */
uint32_t pmm_total_memory() {
	return mem_size;
}

/* Mark an area of physical memory as available.
 */
void pmm_init_region(uintptr_t addr, uint32_t size) {
	// block number this region starts at
	uint32_t base_block = addr/PMM_BLOCK_SIZE;

	// number of blocks this region represents
	uint32_t num = divide_up(size, PMM_BLOCK_SIZE);

	while (num-- > 0) {
		mmap_unset(base_block++);
	}

	// Never map the nullptr
	mmap_set(0);
}

/* Mark an area of physical memory as used.
 */
void pmm_deinit_region(uintptr_t addr, uint32_t size) {
	uint32_t base_block = addr/PMM_BLOCK_SIZE;
	uint32_t num = divide_up(size, PMM_BLOCK_SIZE);

	while (num-- > 0) {
		mmap_set(base_block++);
	}
}

/* Returns the address of a free page of physical memory.
 * Note: of course, this address is page-aligned.
 */
uintptr_t pmm_alloc_page() {
	if (max_blocks - used_blocks <= 0) {
		printf("[PMM] Kernel is out of physical memory!");
		abort();
	}

	uint32_t block = mmap_find_free();

	if (!block)  {
		return 0;
	}

	mmap_set(block);

	return (uintptr_t) (block*PMM_BLOCK_SIZE);
}

/* Returns the address of a 4 MiB area of physical memory, aligned to 4 MiB.
 *
 * Strategy:
 * Find an 8 MiB section of memory. In there, we are guaranteed to find an
 * address that is 4 MiB-aligned. Return that, mark 4 MiB as taken.
 */
uintptr_t pmm_alloc_aligned_large_page() { // TODO: generalize
	if (max_blocks - used_blocks < 2*1024) { // 4MiB
		return 0;
	}

	uint32_t free_block = mmap_find_free_frame(2*1024);

	if (!free_block) {
		return 0;
	}

	uint32_t aligned_block = ((free_block / 1024) + 1) * 1024;

	for (int i = 0; i < 1024; i++) {
		mmap_set(aligned_block + i);
	}

	return (uintptr_t)(aligned_block*PMM_BLOCK_SIZE);
}

uintptr_t pmm_alloc_pages(uint32_t num) {
	if (max_blocks-used_blocks < num) {
		return 0;
	}

	uint32_t first_block = mmap_find_free_frame(num);

	if (!first_block) {
		return 0;
	}

	for (uint32_t i = 0; i < num; i++) {
		mmap_set(first_block+i);
	}

	return (uintptr_t) (first_block*PMM_BLOCK_SIZE);
}

void pmm_free_page(uintptr_t addr) {
	uint32_t block = addr/PMM_BLOCK_SIZE;
	mmap_unset(block);
}

void pmm_free_pages(uintptr_t addr, uint32_t num) {
	uint32_t first_block = addr/PMM_BLOCK_SIZE;

	for (uint32_t i = 0; i < num; i++) {
		mmap_unset(first_block+i);
	}
}

void mmap_set(uint32_t bit) {
	bitmap[bit / 32] |= NTHBIT(bit % 32);
	used_blocks++;
}

void mmap_unset(uint32_t bit) {
	bitmap[bit / 32] &= ~NTHBIT(bit % 32);
	used_blocks--;
}

uint32_t mmap_test(uint32_t bit) {
	return bitmap[bit / 32] & NTHBIT(bit % 32);
}

/* Returns the index of the first free bit in the bitmap
 */
uint32_t mmap_find_free() {
	for (uint32_t i = 0; i < max_blocks/32; i++) {
		if (bitmap[i] != 0xFFFFFFFF) {
			for (uint32_t j = 0; j < 32; j++) {
				if (!(bitmap[i] & NTHBIT(j))) {
					return i * 32 + j;
				}
			}
		}
	}

	return 0;
}

/* Returns the first block of frame_size bits
 */
uint32_t mmap_find_free_frame(uint32_t frame_size) {
	uint32_t first = 0;
	uint32_t count = 0;

	for (uint32_t i = 0; i < max_blocks/32; i++) {
		if (bitmap[i] != 0xFFFFFFFF) {
			for (uint32_t j = 0; j < 32; j++) {
				if (!(bitmap[i] & NTHBIT(j))) {
					if (!first) {
						first = i*32+j;
					}

					count++;
				} else {
					first = 0;
					count = 0;
				}

				if (count == frame_size) {
					return first;
				}
			}
		} else {
			first = 0;
			count = 0;
		}
	}

	return 0;
}

/* Returns the first address after the kernel in physical memory.
 */
uintptr_t pmm_get_kernel_end() {
	return (uintptr_t) kernel_end + max_blocks / 8;
}