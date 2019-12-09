#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>
#include <stddef.h>

__attribute__((__noreturn__))
void abort();

#ifndef _KERNEL_
__attribute__((__noreturn__))
void exit(int status);

void* malloc(size_t size);
void* aligned_alloc(size_t alignment, size_t size);
#endif

char* itoa(int value, char* str, int base);
int atoi(const char* s);

int abs(int n);

#endif
