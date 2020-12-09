#ifndef _KERNEL_

#include <kernel/uapi/uapi_syscall.h>

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

int system(const char* command) {
    return syscall1(SYS_EXEC, (uintptr_t) command);
}

#endif