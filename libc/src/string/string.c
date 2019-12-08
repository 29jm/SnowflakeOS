#include <string.h>
#include <stdint.h>

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