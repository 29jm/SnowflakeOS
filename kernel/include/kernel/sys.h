#pragma once

#define UNUSED(param) (void) param

#define BREAK() do { \
                	asm ("xchgw %bx, %bx\n"); \
                } while (false)

/* Returns the next multiple of `s` greater than `a`, or `a` if it is a
 * multiple of `s`.
 */
static uint32_t align_to(uint32_t n, uint32_t align) {
    if (n % align == 0) {
        return n;
    }

    return n + (align - n % align);
}