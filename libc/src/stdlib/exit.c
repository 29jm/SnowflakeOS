#ifndef _KERNEL_

#include <stdlib.h>

void exit(int status) {
    asm volatile (
        "mov $1, %%eax\n"
        "mov %[status], %%ebx\n"
        "int $0x30\n"
        :: [status] "r" (status)
        : "%eax"
    );

    __builtin_unreachable();
}

#endif