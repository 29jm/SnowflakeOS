#include <sys/stat.h>

#include <kernel/uapi/uapi_syscall.h>
#include <kernel/uapi/uapi_fs.h>

extern int32_t syscall1(uint32_t eax, uint32_t ebx);
extern int32_t syscall2(uint32_t eax, uint32_t ebx, uint32_t ecx);

int mkdir(const char* pathname, mode_t mode) {
    uint32_t inode = syscall2(SYS_MKDIR, (uintptr_t) pathname, mode);

    if (inode == FS_INVALID_INODE) {
        return -1;
    }

    return 0;
}

int chdir(const char* path) {
    return syscall1(SYS_CHDIR, (uintptr_t) path);
}