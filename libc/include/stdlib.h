#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>

__attribute__((__noreturn__))
void abort();

#ifndef _KERNEL_
__attribute__((__noreturn__))
void exit(int status);
#endif

char* itoa(int value, char* str, int base);
int atoi(const char* s);

#endif
