#pragma once

#include <stdbool.h>

#define UNUSED(param) (void) param

#define BREAK() do { \
                    asm ("xchgw %bx, %bx\n"); \
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