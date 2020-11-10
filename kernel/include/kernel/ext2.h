#pragma once

#include <kernel/fs.h>

#include <stdint.h>

#define EXT2_MAGIC 0xEF53
#define EXT2_ROOT_INODE 2

typedef struct inode_t inode_t;

void init_ext2(uint8_t* data, uint32_t len);
uint32_t ext2_create(const char* name, uint32_t type, uint32_t parent_inode);
uint32_t ext2_mkdir(const char* name, uint32_t parent_inode);
uint32_t ext2_open(const char* path);
uint32_t ext2_read(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t size);
uint32_t ext2_append(uint32_t inode, uint8_t* data, uint32_t size);
sos_directory_entry_t* ext2_readdir(uint32_t inode, uint32_t offset);
uint32_t ext2_get_file_size(uint32_t inode);
inode_t* ext2_get_fs_inode(uint32_t inode);