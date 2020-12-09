#include <stdio.h>

#include <kernel/uapi/uapi_syscall.h>

#ifndef _KERNEL_

extern int32_t syscall2(uint32_t eax, uint32_t ebx, uint32_t ecx);

int rename(const char* old, const char* new) {
    return syscall2(SYS_RENAME, (uintptr_t) old, (uintptr_t) new);
}

/* TODO: call `rmdir` on directories.
 */
int remove(const char* path) {
    return unlink(path);
}

int fflush(FILE* stream) {
    (void) stream;

    return 0;
}

#endif