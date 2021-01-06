#pragma once

#include <kernel/multiboot2.h>

#include <stdint.h>

void init_pmm(mb2_t* boot);
uint32_t pmm_used_memory();
uint32_t pmm_total_memory();
void pmm_init_region(uintptr_t addr, uint32_t size);
void pmm_deinit_region(uintptr_t addr, uint32_t size);
uintptr_t pmm_alloc_page();
uintptr_t pmm_alloc_aligned_large_page();
uintptr_t pmm_alloc_pages(uint32_t num);
void pmm_free_page(uintptr_t addr);
void pmm_free_pages(uintptr_t addr, uint32_t num);
uintptr_t pmm_get_kernel_end();

extern uint32_t* mem_map;

#define PMM_BLOCK_SIZE 4096