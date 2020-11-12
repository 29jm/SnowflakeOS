#pragma once

#include <kernel/fs.h>

#include <stdint.h>

#define EXT2_MAGIC 0xEF53
#define EXT2_ROOT_INODE 2

fs_t* init_ext2(uint8_t* data, uint32_t len);