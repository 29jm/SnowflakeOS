#include <stdio.h>

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_NOFLOAT
#define STB_SPRINTF_NOUNALIGNED
#define STB_SPRINTF_MIN 512
#include <deps/stb_sprintf.h>

static FILE __stdout = (FILE) {STDOUT_FILENO, "stdout"};

// No such thing as stderr right now
FILE* stdout = &__stdout;
FILE* stderr = &__stdout;

static char* callback(const char* buf, void* fd, int len) {
    (void) fd;

#ifndef _KERNEL_
    fwrite(buf, 1, len, fd);
#endif

    for (int i = 0; i < len; i++) {
        putchar((int) buf[i]);
    }

    return (char*) buf;
}

int sprintf(char* str, const char* format, ...) {
    va_list ap;

    va_start(ap, format);
    int ret = vsprintf(str, format, ap);
    va_end(ap);

    return ret;
}

int snprintf(char* str, size_t size, const char* format, ...) {
    va_list ap;

    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);

    return ret;
}

int vsprintf(char* str, const char* format, va_list ap) {
    return stbsp_vsprintf(str, format, ap);
}

int vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    return stbsp_vsnprintf(str, size, format, ap);
}

int vfprintf(FILE* stream, const char* format, va_list ap) {
    char buf[STB_SPRINTF_MIN];
    return stbsp_vsprintfcb(callback, (void*) stream, buf, format, ap);
}

int fprintf(FILE* fd, const char* format, ...) {
    va_list ap;

    va_start(ap, format);
    int ret = vfprintf(fd, format, ap);
    va_end(ap);

    return ret;
}

int printf(const char* format, ...) {
    va_list ap;

    va_start(ap, format);
    int ret = vfprintf(stdout, format, ap);
    va_end(ap);

    return ret;
}