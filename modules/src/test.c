#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <snow.h>

int main() {
	uint32_t* buff = (uint32_t*) snow_alloc(768);

	printf("TEST: alloc at 0x%X\n", (unsigned int) buff);

	for (uint32_t i = 0; i < 1024*768; i++) {
		buff[i] = i | i*512 | i % 512;
	}

	snow_render((uintptr_t) buff);

	while (true);

	return 0;
}
