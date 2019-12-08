#ifndef SNOWLIB_H
#define SNOWLIB_H

#include <stdint.h>

void* snow_alloc(uint32_t size);
void snow_render(uintptr_t buffer);

#endif