#include <stdlib.h>

#ifndef _KERNEL_

#include <snow.h>

static uintptr_t heap_pointer;
static uint32_t remaining_bytes;

/* Returns an `alignment` bytes-aligned pointer to a newly allocated memory location
 * of size `size`.
 */
void* aligned_alloc(size_t alignment, size_t size) {
	if (heap_pointer == 0) {
		heap_pointer = (uintptr_t) snow_alloc(1);
		remaining_bytes = 0x1000;
	}

	uintptr_t next = ((heap_pointer/alignment) + 1) * alignment;
	remaining_bytes -= next - heap_pointer;

	if (size > remaining_bytes) {
		uint32_t pages_needed = (size - remaining_bytes)/0x1000 + 1;
		snow_alloc(pages_needed);
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