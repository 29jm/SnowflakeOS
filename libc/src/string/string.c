#include <string.h>
#include <stdint.h>
#include <stdlib.h>

uint32_t strlen(const char* string) {
	uint32_t result = 0;

	while (string[result])
		result++;

	return result;
}

char* strcpy(char* dest, const char* src) {
	for (uint32_t i = 0; i < strlen(src); i++) {
		dest[i] = src[i];
	}

	dest[strlen(src)] = 0; // Null terminator

	return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
	uint32_t len = strlen(src);
	uint32_t max = len > n ? n : len;

	for (uint32_t i = 0; i < max; i++) {
		dest[i] = src[i];
	}

	return dest;
}

char* strdup(const char* s) {
#ifndef _KERNEL_
	char* buff = (char*) malloc((strlen(s)+1)*sizeof(char));
#else
	char* buff = (char*) kmalloc((strlen(s)+1)*sizeof(char));
#endif

	return strcpy(buff, s);
}

char* strchrnul(const char* s, int c) {
	int n = strlen(s);

	for (int i = 0; i < n; i++) {
		if (s[i] == c) {
			return (char*) &s[i];
		}
	}

	return (char*) &s[n];
}

int strcmp(const char* s1, const char* s2) {
	while (*s1 && *s1 == *s2) {
		s1++;
		s2++;
	}

	return *(uint8_t*) s1 - *(uint8_t*) s2;
}