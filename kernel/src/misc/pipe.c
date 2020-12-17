#include <kernel/pipe.h>
#include <kernel/sys.h>

#include <stdlib.h>
#include <string.h>

#define PIPE_SIZE 2048

typedef struct pipe_fs_t {
    fs_t fs;
    uint8_t* buf;
    uint32_t buf_size;
    uint32_t content_start;
    uint32_t content_size;
} pipe_fs_t;

uint32_t pipe_read(pipe_fs_t* pipe, uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t size) {
    UNUSED(inode); // There's only one "file" in this fs, the pipe itself
    UNUSED(offset);

    for (uint32_t i = 0; i < size; i++) {
        if (!pipe->content_size) {
            return i;
        }

        buf[i] = pipe->buf[pipe->content_start];
        pipe->content_start = (pipe->content_start + 1) % pipe->buf_size;
        pipe->content_size--;
    }

    return size;
}

uint32_t pipe_append(pipe_fs_t* pipe, uint32_t inode, uint8_t* data, uint32_t size) {
    UNUSED(inode);

    for (uint32_t i = 0; i < size; i++) {
        if (pipe->content_size == PIPE_SIZE) {
            pipe->buf[pipe->content_start] = data[i];
            pipe->content_start = (pipe->content_start + 1) % pipe->buf_size;
        } else {
            pipe->buf[(pipe->content_start + pipe->content_size) % pipe->buf_size] = data[i];
            pipe->content_size++;
        }
    }

    return size;
}

int32_t pipe_close(pipe_fs_t* fs, uint32_t ino) {
    UNUSED(ino);

    kfree(fs->buf);
    kfree(fs->fs.root);
    kfree(fs);

    return 0;
}

inode_t* pipe_new() {
    // TODO: have root inodes not necessarily be folders?
    inode_t* p = zalloc(sizeof(folder_inode_t));
    p->fs = zalloc(sizeof(pipe_fs_t));

    pipe_fs_t* fs = (pipe_fs_t*) p->fs;
    fs->buf = zalloc(PIPE_SIZE);
    fs->buf_size = PIPE_SIZE;

    p->fs->root = (folder_inode_t*) p;
    p->fs->read = (fs_read_t) pipe_read;
    p->fs->append = (fs_append_t) pipe_append;
    p->fs->close = (fs_close_t) pipe_close;

    return p;
}