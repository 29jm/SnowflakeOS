#pragma once

#include <stdint.h>

#define EXT2_MAGIC 0xEF53
#define EXT2_ROOT_INODE 2

typedef struct {
	uint32_t inode;
	uint16_t entry_size;
	uint8_t name_len_low;
	uint8_t type;
	char name[];
} ext2_directory_entry_t;

void init_ext2(uint8_t* data, uint32_t len);
uint32_t ext2_create(const char* name, uint32_t parent_inode);
uint32_t ext2_mkdir(const char* name, uint32_t parent_inode);
uint32_t ext2_open(const char* path);
uint32_t ext2_read(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t size);
uint32_t ext2_append(uint32_t inode, uint8_t* data, uint32_t size);
ext2_directory_entry_t* ext2_readdir(uint32_t inode, uint32_t offset);