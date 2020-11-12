#pragma once

#include <kernel/uapi/uapi_fs.h>

#include <stdint.h>
#include <stdbool.h>
#include <list.h>

#define O_CREATD 32

typedef enum inode_type_t {
    inode_type_file = DENT_FILE,
    inode_type_folder = DENT_DIRECTORY
} inode_type_t;

typedef struct inode_t {
    uint32_t inode_no;
    uint32_t size;
    inode_type_t type;
    uint32_t hardlinks;
} inode_t;

typedef struct file_inode_t {
    inode_t ino;
} file_inode_t;

typedef struct folder_inode_t {
    inode_t ino;
    bool dirty;
    list_t subfolders;
    list_t subfiles;
} folder_inode_t;

void init_fs();
char* fs_normalize_path(const char* p);
inode_t* fs_open(const char* path, uint32_t mode);
uint32_t fs_mkdir(const char* path, uint32_t mode);
int32_t fs_unlink(const char* path);
void fs_close(uint32_t fd);
uint32_t fs_read(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t size);
uint32_t fs_write(uint32_t fd, uint8_t* buf, uint32_t size);
uint32_t fs_readdir(uint32_t fd, uint32_t offset, sos_directory_entry_t* d_ent, uint32_t size);