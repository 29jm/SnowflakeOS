#include <stdlib.h>

void exit(int status) {
#ifndef _KERNEL_
    asm (
        "mov %[status], %%eax\n"
        "int $0x30\n"
        :
        : [status] "r" (status)
        : "%eax"
    );
#endif
}