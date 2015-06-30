#include <string.h>

int memcmp(const void* aptr, const void* bptr, size_t size) {
	const unsigned char* a = (const unsigned char*) aptr;
	const unsigned char* b = (const unsigned char*) bptr;
	for (size_t i = 0; i < size; i++)
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
	return 0;
}
