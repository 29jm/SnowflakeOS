#include <stdlib.h>
#include <stdio.h>

#ifndef _KERNEL_

/* Returns the next multiple of `s` greater than `a`, or `a` if it is a
 * multiple of `s`.
 */
#define ALIGN(a, s) (((a)/(s) + ((a) % (s) ? 1 : 0)) * (s))

/* Allocates `n` pages of memory located after the program's memory.
 * Returns a pointer to the first allocated page.
 */
static void* sbrk(uint32_t size) {
	uintptr_t address;

	asm volatile (
		"mov $4, %%eax\n"
		"mov %[size], %%ecx\n"
		"int $0x30\n"
		"mov %%eax, %[address]\n"
		: [address] "=r" (address)
		: [size] "r" (size)
		: "%eax", "%ecx"
	);

	return (void*) address;
}

/* Returns a pointer to a newly allocated memory location of size `size` with
 * the specified alignment.
 */
void* aligned_alloc(size_t alignment, size_t size) {
	uintptr_t heap_pointer = (uintptr_t) sbrk(0);

	uintptr_t next = ALIGN(heap_pointer, alignment);
	uint32_t needed = next - heap_pointer + size;

	if (sbrk(needed) == (void*) -1) {
		return (void*) -1;
	}

	return (void*) next;
}

/* Allocates `size` bytes of memory.
 */
void* malloc(size_t size) {
	return aligned_alloc(4, size);
}

#endif // #ifndef _KERNEL_