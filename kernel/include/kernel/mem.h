#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct _mem_block_t {
    struct _mem_block_t* next;
    uint32_t size; // We use the last bit as a 'used' flag
    uint8_t data[1];
} mem_block_t;

void* kmalloc(uint32_t size);
void* kamalloc(uint32_t size, uint32_t align);

void kfree(void* pointer);