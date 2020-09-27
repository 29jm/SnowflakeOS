#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/uapi/uapi_syscall.h>

extern int32_t syscall3(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);
extern int32_t syscall2(uint32_t eax, uint32_t ebx, uint32_t ecx);
extern int32_t syscall1(uint32_t eax, uint32_t ebx);

FILE* fopen(const char* path, const char* mode) {
    if (strcmp(mode, "r")) {
        return NULL; // TODO
    }

    int m = 1; // TODO use param
    int32_t fd = -1;

    fd = syscall2(SYS_OPEN, (uintptr_t) path, m);

    FILE* stream = malloc(sizeof(FILE));
    stream->fd = fd;
    stream->name = strdup(path);

    return stream;
}

int fclose(FILE* stream) {
    if (!stream) {
        return -1;
    }

    syscall1(SYS_CLOSE, stream->fd);

    free(stream->name);
    free(stream);

    return 0;
}

int fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    uint32_t read = 0;

    read = syscall3(SYS_READ, stream->fd, (uintptr_t) ptr, size*nmemb);

    return read / size;
}

int fgetc(FILE* stream) {
    int c = 0;

    fread(&c, 1, 1, stream);

    return c;
}