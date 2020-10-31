#include <kernel/fs.h>
#include <kernel/ext2.h>
#include <kernel/proc.h>
#include <kernel/list.h>
#include <kernel/sys.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static list_t* file_table;
static uint32_t current_fd;

void init_fs() {
    file_table = list_new();
}

/* Returns the absolute version of `p`, free of oddities,
 * dynamically allocated.
 */
char* normalize_path(const char* p) {
    char* np = kmalloc(MAX_PATH);
    strcpy(np, p);

    if (!strcmp(np, "/")) {
        return np;
    }

    // Make the path absolute
    if (p[0] != '/') {
        char* cwd = proc_get_cwd();
        strcpy(np, cwd);
        strcat(np, "/");
        strcat(np, p);
        kfree(cwd);
    }

    // Trim trailing slashes
    while (np[strlen(np) - 1] == '/') {
        np[strlen(np) - 1] = '\0';
    }

    // Trim '//' sequences
    char* s = NULL;
    while ((s = strstr(np, "//"))) {
        uint32_t n = 0;

        // Make sure to keep exaclty one /
        if (strstr(np, "///") != s) {
            n = 1;
        } else {
            n = 2;
        }

        char* npp = kmalloc(MAX_PATH);
        strncpy(npp, np, s - np);
        npp[s - np] = '\0';
        strcat(npp, s + n);
        kfree(np);
        np = npp;
    }

    return np;
}

/* Returns the path to the parent directory of the thing pointed to by `p`,
 * dynamically allocated.
 * Expects `p` to be a normalized path.
 */
char* dirname(const char* p) {
    char* last_sep = strrchr(p, '/');
    char* dp = kmalloc(MAX_PATH);

    strcpy(dp, p);

    if (!strcmp(dp, "/")) {
        return dp;
    }

    if (last_sep == p) {
        dp[1] = '\0';
    } else {
        dp[last_sep - p] = '\0';
    }

    return dp;
}

/* Returns the name of the file pointed to by `p`.
 */
char* basename(const char* p) {
    char* last_sep = strrchr(p, '/');

    if (!strcmp(p, "/")) {
        return (char*) p;
    }

    return last_sep + 1;
}

uint32_t fs_open(const char* path, uint32_t mode) {
    char* npath = normalize_path(path);
    uint32_t inode = 0;

    if (mode & O_CREAT) {
        char* parent_path = dirname(path);
        char* filename = basename(path);
        uint32_t parent_inode = ext2_open(parent_path);
        inode = ext2_create(filename, parent_inode);
        kfree(parent_path);
    } else if (mode & O_RDONLY) {
        inode = ext2_open(path);
    }

    kfree(npath);

    if (!inode) {
        return FS_INVALID_FD;
    }

    // TODO: check that it's not already opened

    ft_entry_t* entry = kmalloc(sizeof(ft_entry_t));

    entry->fd = ++current_fd;
    entry->inode = inode;
    entry->mode = mode;
    entry->offset = 0;

    list_add(file_table, entry);

    return entry->fd;
}

uint32_t fs_mkdir(const char* path, uint32_t mode) {
    UNUSED(mode);

    uint32_t inode = 0;
    char* np = normalize_path(path);

    char* parent_path = dirname(np);
    char* filename = basename(np);
    uint32_t parent_inode = ext2_open(parent_path);

    inode = ext2_mkdir(filename, parent_inode);

    kfree(parent_path);
    kfree(np);

    if (!inode) {
        return FS_INVALID_INODE;
    }

    return inode;
}

void fs_print_open() {
    printf("[fs] open file descriptors: ");

    for (uint32_t i = 0; i < file_table->count; i++) {
        printf("%d -> ", ((ft_entry_t*) list_get_at(file_table, i))->fd);
    }

    printf("end\n");
}

void fs_close(uint32_t fd) {
    for (uint32_t i = 0; i < file_table->count; i++) {
        ft_entry_t* entry = list_get_at(file_table, i);

        if (entry->fd == fd) {
            list_remove_at(file_table, i);
            kfree(entry);
            return;
        }
    }
}

uint32_t fs_read(uint32_t fd, uint8_t* buf, uint32_t size) {
    for (uint32_t i = 0; i < file_table->count; i++) {
        ft_entry_t* entry = list_get_at(file_table, i);

        if (entry->fd == fd && (entry->mode & O_RDONLY || entry->mode & O_RDWR)) {
            uint32_t read = ext2_read(entry->inode, entry->offset, buf, size);
            entry->offset += read;

            if (read < size) {
                buf[read] = (uint8_t) -1; // EOF
            }

            return read;
        }
    }

    return 0;
}

uint32_t fs_write(uint32_t fd, uint8_t* buf, uint32_t size) {
    for (uint32_t i = 0; i < file_table->count; i++) {
        ft_entry_t* entry = list_get_at(file_table, i);

        if (entry->fd == fd && entry->mode & O_APPEND) {
            uint32_t written = ext2_append(entry->inode, buf, size);
            entry->offset += written;

            return written;
        }
    }

    return 0;
}

uint32_t fs_readdir(uint32_t fd, sos_directory_entry_t* d_ent, uint32_t size) {
    for (uint32_t i = 0; i < file_table->count; i++) {
        ft_entry_t* entry = list_get_at(file_table, i);

        if (entry->fd == fd && entry->mode & O_RDONLY) {
            ext2_directory_entry_t* ent = ext2_readdir(entry->inode, entry->offset);

            // TODO: check if empty entries aren't actually valid
            if (ent == NULL || ent->name[0] == '\0') {
                return 0;
            }

            uint32_t unpadded_size = min(ent->entry_size, 263);

            if (unpadded_size < size) {
                // TODO: proper assignation depending on fs type
                memcpy(d_ent, ent, unpadded_size);
                d_ent->name[d_ent->name_len_low] = '\0';
            } else {
                kfree(ent);

                return 0;
            }

            entry->offset += d_ent->entry_size;
            kfree(ent);

            return d_ent->entry_size;
        }
    }

    return 0;
}