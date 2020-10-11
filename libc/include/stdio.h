#pragma once

#include <stdint.h>
#include <stddef.h>

#define EOF -1

typedef struct {
    int32_t fd;
    char* name;
} FILE;

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

#ifndef _KERNEL_
FILE* fopen(const char* path, const char* mode);
int fclose(FILE* stream);
int fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
int fgetc(FILE* stream);
int fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int fputc(int c, FILE* stream);
#endif