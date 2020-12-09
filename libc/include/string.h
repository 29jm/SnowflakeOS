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
size_t strnlen(const char* s, size_t maxlen);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strdup(const char* src);
char* strndup(const char* s, size_t n);
char* strchr(const char* s, int c);
char* strchrnul(const char* s, int c);
char* strrchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);

#ifdef __cplusplus
}
#endif