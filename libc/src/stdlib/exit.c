#ifndef _KERNEL_

#include <kernel/uapi/uapi_syscall.h>

#include <stdlib.h>

int32_t syscall1(uint32_t eax, uint32_t ebx);
int32_t syscall2(uint32_t eax, uint32_t ebx, uint32_t ecx);

void exit(int status) {
    syscall1(SYS_EXIT, status);
    __builtin_unreachable();
}

int system(const char* command) {
    return syscall2(SYS_EXEC, (uintptr_t) command, 0);
}

#endif