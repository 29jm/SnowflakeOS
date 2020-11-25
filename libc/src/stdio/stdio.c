#include <stdio.h>

#include <kernel/uapi/uapi_syscall.h>

extern int32_t syscall2(uint32_t eax, uint32_t ebx, uint32_t ecx);

int rename(const char* old, const char* new) {
    return syscall2(SYS_RENAME, (uintptr_t) old, (uintptr_t) new);
}