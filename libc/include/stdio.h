#pragma once

#include <stdint.h>
#include <stddef.h>

#define EOF ((uint8_t) -1)

typedef struct {
    int32_t fd;
    char* name;
} FILE;

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);
FILE* fopen(const char* path, const char* mode);
int fclose(FILE* stream);
int fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
int fgetc(FILE* stream);