#pragma once

#include <stdint.h>

void init_stacktrace(uint8_t* data, uint32_t size);
void stacktrace_print();