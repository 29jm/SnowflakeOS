#pragma once

#include <kernel/fs.h>

typedef struct pipe_t {
    inode_t inode;
    uint8_t* buf;
    uint32_t index;
} pipe_t;

inode_t* pipe_new();
inode_t* pipe_clone(inode_t* pipe);