#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void* a, const void* b, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
void* memset(void* mem, int val, size_t n);
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strdup(const char* src);

#ifdef __cplusplus
}
#endif