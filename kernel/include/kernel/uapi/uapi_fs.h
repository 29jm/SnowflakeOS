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

#define DENT_INVALID 0
#define DENT_FILE 1
#define DENT_DIRECTORY 2

#define FS_STDOUT_FILENO 1

typedef struct {
    uint32_t inode;
    uint16_t entry_size;
    uint8_t name_len_low;
    uint8_t type;
    char name[];
} sos_directory_entry_t;

typedef struct stat_t {
    uint32_t st_dev;
    uint32_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_size;
} stat_t;