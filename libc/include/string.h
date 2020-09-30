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
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strdup(const char* src);
char* strchrnul(const char* s, int c);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);

#ifdef __cplusplus
}
#endif