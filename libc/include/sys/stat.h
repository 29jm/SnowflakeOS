#pragma once

#include <stdint.h>

typedef uint32_t mode_t;

struct stat {
    uint32_t st_dev;
    uint32_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_size;
};

#ifndef _KERNEL_
int mkdir(const char* pathname, mode_t mode);
int stat(const char* path, struct stat* buf);
#endif