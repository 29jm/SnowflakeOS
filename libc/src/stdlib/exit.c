#include <stdlib.h>

#ifdef _KERNEL_

#include <kernel/sys.h>

#endif

void exit(int status) {
#ifndef _KERNEL_
    asm (
        "mov $1, %%eax\n"
        "mov %[status], %%ebx\n"
        "int $0x30\n"
        :
        : [status] "r" (status)
        : "%eax"
    );

    __builtin_unreachable();
#else
    UNUSED(status);
#endif
}