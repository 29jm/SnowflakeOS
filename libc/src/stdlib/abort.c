#include <stdio.h>

#ifndef _KERNEL_
#include <stdlib.h>
#else
#include <kernel/stacktrace.h>
#endif

__attribute__((__noreturn__))
void abort()
{
#ifdef _KERNEL_
    printf("Kernel Panic: abort()\n");
    stacktrace_print();
    while (1) {}
    __builtin_unreachable();
#else
    exit(1);
#endif
}