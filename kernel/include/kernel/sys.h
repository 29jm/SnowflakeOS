#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define UNUSED(param) (void) param
#define PHYS_TO_VIRT(addr) ((addr) + KERNEL_BASE_VIRT)
#define VIRT_TO_PHYS(addr) ((addr) - KERNEL_BASE_VIRT)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define printk(format, ...) printf("[\x1B[32m%s\x1B[0m] " format "\n", __FILENAME__, ##__VA_ARGS__)

#define printke(format, ...) \
    printf("[\x1B[31;1m%s\x1B[0m] " format "\n", __FILENAME__, ##__VA_ARGS__)

#define BREAK() \
    do { \
        asm("xchgw %bx, %bx\n"); \
    } while (false)

/* Returns the next multiple of `align` greater than `n`, or `n` if it is a
 * multiple of `align`.
 */
static uint32_t align_to(uint32_t n, uint32_t align) {
    if (n % align == 0) {
        return n;
    }

    return n + (align - n % align);
}

/* When you can't divide a person in half.
 */
static uint32_t divide_up(uint32_t n, uint32_t d) {
    if (n % d == 0) {
        return n / d;
    }

    return 1 + n / d;
}

/* Dumps any contiguous memory structure's bytes as a string of hex octets with
 * position numbers to aid in debugging efforts.
 */
inline void dbg_buffer_dump(void* buff, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%d:%#2X ", i, ((uint8_t*) buff)[i]);
    }

    printf("\n");
}