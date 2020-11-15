#pragma once

#include <kernel/uapi/uapi_fs.h>

#include <stdint.h>
#include <stdbool.h>
#include <list.h>

#define O_CREATD 32

typedef struct fs_t fs_t;

typedef enum inode_type_t {
    inode_type_file = DENT_FILE,
    inode_type_folder = DENT_DIRECTORY
} inode_type_t;

typedef struct inode_t {
    uint32_t inode_no;
    uint32_t size;
    inode_type_t type;
    uint32_t hardlinks;
    fs_t* fs;
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

typedef struct fs_t {
    folder_inode_t* root;
    uint32_t uid;
    uint32_t (*create)(struct fs_t*, const char*, uint32_t, uint32_t);
    int32_t (*unlink)(struct fs_t*, uint32_t, uint32_t);
    uint32_t (*read)(struct fs_t*, uint32_t, uint32_t, uint8_t*, uint32_t);
    uint32_t (*append)(struct fs_t*, uint32_t, uint8_t*, uint32_t);
    /* TODO: require null termination of entries */
    sos_directory_entry_t* (*readdir)(struct fs_t*, uint32_t, uint32_t);
    inode_t* (*get_fs_inode)(struct fs_t*, uint32_t);
} fs_t;

typedef inode_t* (*fs_get_fs_inode_t)(struct fs_t*, uint32_t);
typedef sos_directory_entry_t* (*fs_readdir_t)(struct fs_t*, uint32_t, uint32_t);
typedef uint32_t (*fs_append_t)(struct fs_t*, uint32_t, uint8_t*, uint32_t);
typedef uint32_t (*fs_read_t)(struct fs_t*, uint32_t, uint32_t, uint8_t*, uint32_t);
typedef int32_t (*fs_unlink_t)(struct fs_t*, uint32_t, uint32_t);
typedef uint32_t (*fs_create_t)(struct fs_t*, const char*, uint32_t, uint32_t);

void init_fs(fs_t* fs);
void fs_mount(const char* mount_point, fs_t* fs);
char* fs_normalize_path(const char* p);
inode_t* fs_open(const char* path, uint32_t mode);
uint32_t fs_mkdir(const char* path, uint32_t mode);
int32_t fs_unlink(const char* path);
void fs_close(inode_t* in);
uint32_t fs_read(inode_t* in, uint32_t offset, uint8_t* buf, uint32_t size);
uint32_t fs_write(inode_t* in, uint8_t* buf, uint32_t size);
uint32_t fs_readdir(inode_t* in, uint32_t offset, sos_directory_entry_t* d_ent, uint32_t size);