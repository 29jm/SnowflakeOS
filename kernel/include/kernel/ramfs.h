#pragma once

#include <kernel/fs.h>

typedef struct ramfs_t {
    uint8_t* data;
    uint32_t size;
} ramfs_t;

fs_device_t ramfs_new(uint8_t* data, uint32_t size);
