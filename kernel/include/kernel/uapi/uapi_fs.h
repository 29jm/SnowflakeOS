#pragma once

#include <stdint.h>

#define MAX_PATH 256

#define FS_INVALID_FD 0
#define FS_INVALID_INODE 0

#define O_APPEND 1
#define O_CREAT  2
#define O_RDONLY 4
#define O_WRONLY 4
#define O_TRUNC  8
#define O_RDWR   16

#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3

typedef struct {
    uint32_t inode;
    uint16_t entry_size;
    uint8_t name_len_low;
    uint8_t type;
    char name[];
} sos_directory_entry_t;