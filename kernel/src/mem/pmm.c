#include <kernel/pmm.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

static uint32_t mem_size;
static uint32_t used_blocks;
static uint32_t max_blocks;

uint32_t* mem_map;

// Linker-provided symbol
extern uint32_t KERNEL_END;

// mem is in KiB
void init_pmm(uint32_t mem) {
	mem_size = mem;
	mem_map = &KERNEL_END;
	max_blocks = (mem*1024) / PMM_BLOCK_SIZE;
	used_blocks = max_blocks;

	memset(mem_map, 0xFF, max_blocks/8);
}

uint32_t pmm_get_map_size() {
	return max_blocks/8;
}

void pmm_init_region(uintptr_t addr, uint32_t size) {
	// block number this region starts at
	uint32_t base_block = addr/PMM_BLOCK_SIZE;

	// number of blocks this region represents
	uint32_t num = size/PMM_BLOCK_SIZE;

	while (num-- > 0) {
		mmap_unset(base_block++);
	}

	// Never map the nullptr
	mmap_set(0);
}

void pmm_deinit_region(uintptr_t addr, uint32_t size) {
	uint32_t base_block = addr/PMM_BLOCK_SIZE;
	uint32_t num = size/PMM_BLOCK_SIZE;

	printf("[PMM] Reserving blocs %d to %d\n", base_block, base_block+num);

	while (num-- > 0) {
		mmap_set(base_block++);
	}
}

void* pmm_alloc_page() {
	if (max_blocks - used_blocks <= 0) {
		printf("[PMM] Kernel is out of memory!");
		abort();
	}

	uint32_t block = mmap_find_free();

	if (!block)  {
		return 0;
	}

	mmap_set(block);

	return (void*) (block*PMM_BLOCK_SIZE);
}

void* pmm_alloc_pages(uint32_t num) {
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

	return (void*) (first_block*PMM_BLOCK_SIZE);
}

void pmm_free_page(uintptr_t addr) {
	uint32_t block = ((uintptr_t) addr)/PMM_BLOCK_SIZE;
	mmap_unset(block);
}

void pmm_free_pages(uintptr_t addr, uint32_t num) {
	addr += 0xC0000000;
	uint32_t first_block = addr/PMM_BLOCK_SIZE;

	for (uint32_t i = 0; i < num; i++) {
		mmap_unset(first_block+i);
	}
}

void mmap_set(uint32_t bit) {
	mem_map[bit/32] |= (1 << (bit % 32));
	used_blocks++;
}

void mmap_unset(uint32_t bit) {
	mem_map[bit/32] &= ~(1 << (bit % 32));
	used_blocks--;
}

uint32_t mmap_test(uint32_t bit) {
	return mem_map[bit / 32] & (1 << (bit % 32));
}

// Returns the first free bit
uint32_t mmap_find_free() {
	for (uint32_t i = 0; i < max_blocks/32; i++) {
		if (mem_map[i] != 0xFFFFFFFF) {
			for (uint32_t j = 0; j < 32; j++) {
				if (!(mem_map[i] & (1 << j))) {
					return i*32+j;
				}
			}
		}
	}

	return 0;
}

// Returns the first block of frame_size bits
uint32_t mmap_find_free_frame(uint32_t frame_size) {
	uint32_t first = 0;
	uint32_t count = 0;

	for (uint32_t i = 0; i < max_blocks/32; i++) {
		if (mem_map[i]  != 0xFFFFFFFF) {
			for (uint32_t j = 0; j < 32; j++) {
				if (!(mem_map[i] & (1 << j))) {
					if (!first) {
						first = i*32+j;
					}

					count++;
				}
				else {
					first = 0;
					count = 0;
				}

				if (count == frame_size) {
					return first;
				}
			}
		}
	}

	return 0;
}
