#pragma once

#include <stdint.h>

typedef uint32_t mode_t;

int mkdir(const char* pathname, mode_t mode);