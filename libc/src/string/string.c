#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _KERNEL_
#include <kernel/mem.h>
#endif

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

char* strdup(const char* s) {
#ifndef _KERNEL_
	char* buff = (char*) malloc(strlen(s)*sizeof(char));
#else
	char* buff = (char*) kmalloc(strlen(s)*sizeof(char));
#endif

	return strcpy(buff, s);
}