#include <stdlib.h>

#ifndef _KERNEL_

#include <snow.h>

static uintptr_t heap_pointer;
static uint32_t remaining_bytes;

/* Allocates `n` pages of memory located after the program's memory.
 * Returns a pointer to the first allocated page.
 */
static void* alloc_syscall(uint32_t n) {
	uintptr_t address;

	asm volatile (
		"mov $4, %%eax\n"
		"mov %[n], %%ecx\n"
		"int $0x30\n"
		"mov %%eax, %[address]\n"
		: [address] "=r" (address)
		: [n] "r" (n)
		: "%eax", "%ecx"
	);

	return (void*) address;
}

/* Returns an `alignment` bytes-aligned pointer to a newly allocated memory location
 * of size `size`.
 */
void* aligned_alloc(size_t alignment, size_t size) {
	if (heap_pointer == 0) {
		heap_pointer = (uintptr_t) alloc_syscall(1);
		remaining_bytes = 0x1000;
	}

	uintptr_t next = ((heap_pointer/alignment) + 1) * alignment;
	remaining_bytes -= next - heap_pointer;

	if (size > remaining_bytes) {
		uint32_t pages_needed = (size - remaining_bytes)/0x1000 + 1;
		alloc_syscall(pages_needed);
		remaining_bytes += 0x1000*pages_needed;
	}

	heap_pointer = next + size;
	remaining_bytes -= size;

	return (void*) next;
}

/* Allocates `size` bytes of memory.
 */
void* malloc(size_t size) {
	return aligned_alloc(4, size);
}

#endif // #ifndef _KERNEL_