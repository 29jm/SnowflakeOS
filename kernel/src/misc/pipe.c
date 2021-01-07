#include <kernel/pipe.h>
#include <kernel/sys.h>

#include <ringbuffer.h>
#include <stdlib.h>
#include <string.h>

#define PIPE_SIZE 2048

typedef struct pipe_fs_t {
    fs_t fs;
    ringbuffer_t* buf;
} pipe_fs_t;

uint32_t pipe_read(pipe_fs_t* pipe, uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t size) {
    UNUSED(inode); // There's only one "file" in this fs, the pipe itself
    UNUSED(offset);

    return ringbuffer_read(pipe->buf, size, buf);
}

uint32_t pipe_append(pipe_fs_t* pipe, uint32_t inode, uint8_t* data, uint32_t size) {
    UNUSED(inode);

    return ringbuffer_write(pipe->buf, size, data);
}

int32_t pipe_close(pipe_fs_t* fs, uint32_t ino) {
    UNUSED(ino);

    ringbuffer_free(fs->buf);
    kfree(fs->fs.root);
    kfree(fs);

    return 0;
}

inode_t* pipe_new() {
    // TODO: have root inodes not necessarily be folders?
    inode_t* p = zalloc(sizeof(folder_inode_t));
    p->fs = zalloc(sizeof(pipe_fs_t));

    pipe_fs_t* fs = (pipe_fs_t*) p->fs;
    fs->buf = ringbuffer_new(PIPE_SIZE);

    p->fs->root = (folder_inode_t*) p;
    p->fs->read = (fs_read_t) pipe_read;
    p->fs->append = (fs_append_t) pipe_append;
    p->fs->close = (fs_close_t) pipe_close;

    return p;
}