#pragma once

#include <kernel/ext2.h>
#include <kernel/uapi/uapi_fs.h>

#include <stdint.h>

#define FS_MAX_FD 1024

#define FS_READ 1

typedef struct {
    uint32_t fd;
    uint32_t inode;
    uint32_t mode;
    uint32_t offset;
} ft_entry_t;

void init_fs();
uint32_t fs_open(const char* path, uint32_t mode);
void fs_close(uint32_t fd);
uint32_t fs_read(uint32_t fd, uint8_t* buf, uint32_t size);
uint32_t fs_write(uint32_t fd, uint8_t* buf, uint32_t size);
uint32_t fs_readdir(uint32_t fd, sos_directory_entry_t* d_ent, uint32_t size);