#include <stdlib.h>

#ifndef _KERNEL_

void exit(int status) {
    asm (
        "mov $1, %%eax\n"
        "mov %[status], %%ebx\n"
        "int $0x30\n"
        :
        : [status] "r" (status)
        : "%eax"
    );

    __builtin_unreachable();
}

#endif