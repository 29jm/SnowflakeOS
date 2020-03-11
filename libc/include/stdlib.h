#pragma once

#include <stdint.h>
#include <stddef.h>

/* `malloc`-related functions.
 * Those implement an allocator used in both the kernel and in userspace, with
 * few implementation changes between the two.
 * In userspace, they have the usual names, and in the kernel, `free` and
 * `malloc` are renamed to `kfree` and `kmalloc` for clarity.
 */

void* aligned_alloc(size_t alignment, size_t size);

#ifdef _KERNEL_
void* kamalloc(size_t size, size_t align);

#define malloc kmalloc
#define free kfree
#endif

void* malloc(size_t size);
void free(void* ptr);

/* Other functions.
 */

__attribute__((__noreturn__))
void abort();

#ifndef _KERNEL_
__attribute__((__noreturn__))
void exit(int status);
#endif

char* itoa(int value, char* str, int base);
int atoi(const char* s);

int abs(int n);

void srand(unsigned int seed);
int rand();