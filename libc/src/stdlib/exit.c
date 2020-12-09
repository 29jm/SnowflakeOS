#ifndef _KERNEL_

#include <kernel/uapi/uapi_syscall.h>

#include <stdlib.h>

int32_t syscall1(uint32_t eax, uint32_t ebx);

void exit(int status) {
    syscall1(SYS_EXIT, status);
    __builtin_unreachable();
}

int system(const char* command) {
    return syscall1(SYS_EXEC, (uintptr_t) command);
}

#endif