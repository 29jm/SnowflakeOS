#pragma once

#include <stdint.h>

typedef uint32_t mode_t;

#ifndef _KERNEL_
int mkdir(const char* pathname, mode_t mode);
#endif