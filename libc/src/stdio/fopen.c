#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/uapi/uapi_syscall.h>

extern int32_t syscall3(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);
extern int32_t syscall2(uint32_t eax, uint32_t ebx, uint32_t ecx);
extern int32_t syscall1(uint32_t eax, uint32_t ebx);

/* Returns a file handle to the file or directory pointed to by `path`.
 * Returns NULL on error.
 * The file handle must be freed using `fclose`.
 * Note: only the "r" mode is supported as a of now.
 */
FILE* fopen(const char* path, const char* mode) {
    uint32_t m = 0; // TODO use param

    if (!strcmp(mode, "r")) {
        m = 1;
    } else {
        return NULL;
    }

    uint32_t fd = syscall2(SYS_OPEN, (uintptr_t) path, m);

    if (fd == FS_INVALID_FD) {
        return NULL;
    }

    FILE* stream = malloc(sizeof(FILE));
    stream->fd = fd;
    stream->name = strdup(path);

    return stream;
}

/* Closes a file handle previously returned by `fopen`.
 * Returns zero on success.
 */
int fclose(FILE* stream) {
    if (!stream) {
        return -1;
    }

    syscall1(SYS_CLOSE, stream->fd);

    free(stream->name);
    free(stream);

    return 0;
}

/* Reads at most `size*nmemb` into `ptr` from `stream`.
 * Returns the number of elements read.
 * If less than `nmemb` elements are read, EOF is placed after the last
 * byte read.
 */
int fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    uint32_t read = 0;

    read = syscall3(SYS_READ, stream->fd, (uintptr_t) ptr, size*nmemb);

    return read / size;
}

/* Reads a character from `stream` an returns it as an int.
 * Returns EOF if no more bytes are available.
 */
int fgetc(FILE* stream) {
    unsigned char c = 0;

    if (fread(&c, sizeof(c), 1, stream)) {
        return (int) c;
    }

    return EOF;
}