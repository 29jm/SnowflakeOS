#include <kernel/ext2.h>
#include <kernel/fs.h>
#include <kernel/sys.h>

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define EXT2_STATE_CLEAN 1
#define EXT2_STATE_BROKEN 2

#define EXT2_IGNORE 1
#define EXT2_REMOUNT_RO 2
#define EXT2_PANIC 3

enum {
    LINUX, HURD, MASIX, FREEBSD, GENERIC_BSD
};

typedef struct {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t blocks_superuser;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t superblock_block;
    uint32_t block_size;
    uint32_t fragment_size;
    uint32_t blocks_per_group;
    uint32_t fragments_per_group;
    uint32_t inodes_per_group;
    uint32_t last_mount_time;
    uint32_t last_write_time;
    uint16_t mounts_since_fsck;
    uint16_t mounts_before_fsck;
    uint16_t magic;
    uint16_t state;
    uint16_t on_error;
    uint16_t version_minor;
    uint32_t last_fsck;
    uint32_t time_between_fsck;
    uint32_t creator_id;
    uint32_t version_major;
    uint16_t superuser;
    uint16_t supergroup;
    // The following fields are valid iff version_major >= 1
    uint32_t first_inode;
    uint16_t inode_size;
    uint16_t superblock_group;
    uint32_t optional_features;
    uint32_t required_features;
    uint32_t ro_features;
    uint32_t id[4];
    uint32_t name[4];
    uint8_t last_mount_path[64];
    uint32_t compression;
    uint8_t preallocate_files;
    uint8_t preallocate_directories;
    uint16_t unused;
    uint32_t journal_id[4];
    uint32_t journal_inode;
    uint32_t journal_device;
    uint32_t orphans_inode;
} superblock_t;

typedef struct {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocs;
    uint16_t free_inodes;
    uint16_t directories_count;
    uint16_t pad;
    uint32_t unused[3];
} group_descriptor_t;

typedef struct {
    uint16_t type_perms;
    uint16_t uid;
    uint32_t size_lower;
    uint32_t last_access;
    uint32_t creation_time;
    uint32_t last_modified;
    uint32_t deletion_time;
    uint16_t gid;
    uint16_t hardlinks_count;
    uint32_t sectors_used;
    uint32_t flags;
    uint32_t os_specific1;
    uint32_t dbp[12];
    uint32_t sibp;
    uint32_t dibp;
    uint32_t tibp;
    uint32_t generation_number;
    uint32_t extended_attributes;
    uint32_t size_upper;
    uint32_t fragment_block;
    uint32_t os_specific2[3];
} ext2_inode_t;

typedef struct dentry_t {
    char* name;
    uint32_t inode;
    uint32_t type;
} dentry_t;

typedef struct {
    uint32_t inode;
    uint16_t entry_size;
    uint8_t name_len_low;
    uint8_t type;
    char name[];
} ext2_directory_entry_t;

typedef struct ext2_fs_t {
    fs_t fs;
    superblock_t* sb;
    group_descriptor_t* group_descriptors;
    uint8_t* device;
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t num_block_groups;
} ext2_fs_t;

#define INODE_FIFO 0x1000
#define INODE_CHAR 0x2000
#define INODE_DIR 0x4000
#define INODE_BLCK 0x6000
#define INODE_FILE 0x8000
#define INODE_SYM 0xA000
#define INODE_SOCKET 0xC000

#define PERM_OEXEC 0x1
#define PERM_OWRIT 0x2
#define PERM_OREAD 0x4
#define PERM_GEXEC 0x8
#define PERM_GWRIT 0x10
#define PERM_GREAD 0x20
#define PERM_UEXEC 0x40
#define PERM_UWRIT 0x80
#define PERM_UREAD 0x100

#define PERM_STICKY 0x200
#define PERM_SETGID 0x400
#define PERM_SETUID 0x800

#define INODE_TYPE(n) (n & 0xF000)
#define INODE_PERM(n) (n & 0xFFF)

#define DTYPE_INVALID 0
#define DTYPE_FILE 1
#define DTYPE_DIR 2

uint32_t ext2_create(ext2_fs_t* fs, const char* name, uint32_t type, uint32_t parent_inode);
int32_t ext2_unlink(ext2_fs_t* fs, uint32_t d_ino, uint32_t ino);
int32_t ext2_rename(ext2_fs_t* fs, uint32_t dir_ino, uint32_t ino, uint32_t destdir_ino);
uint32_t ext2_mkdir(ext2_fs_t* fs, const char* name, uint32_t parent_inode);
uint32_t ext2_read(ext2_fs_t* fs, uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t size);
uint32_t ext2_append(ext2_fs_t* fs, uint32_t inode, uint8_t* data, uint32_t size);
sos_directory_entry_t* ext2_readdir(ext2_fs_t* fs, uint32_t inode, uint32_t offset);
inode_t* ext2_get_fs_inode(ext2_fs_t* fs, uint32_t inode);
int32_t ext2_close(ext2_fs_t* fs, uint32_t ino);
int32_t ext2_stat(ext2_fs_t* fs, uint32_t ino, stat_t* stat);

static void read_block(ext2_fs_t* fs, uint32_t block, uint8_t* buf);
static void write_block(ext2_fs_t* fs, uint32_t block, uint8_t* buf);
static void clear_block(ext2_fs_t* fs, uint32_t block);
static void write_superblock(ext2_fs_t* fs);
static void write_group_descriptor(ext2_fs_t* fs, uint32_t group);
static superblock_t* parse_superblock(ext2_fs_t* fs);
static group_descriptor_t* parse_group_descriptors(ext2_fs_t* fs);
static void read_inode_block(ext2_fs_t* fs, ext2_inode_t* inode, uint32_t n, uint8_t* buf);
static void write_inode_block(ext2_fs_t* fs, ext2_inode_t* inode, uint32_t n, uint8_t* buf);
static ext2_inode_t* get_inode(ext2_fs_t* fs, uint32_t inode);
static uint32_t allocate_block(ext2_fs_t* fs);
static void free_block(ext2_fs_t* fs, uint32_t block);
static uint32_t allocate_inode(ext2_fs_t* fs);
static void free_inode(ext2_fs_t* fs, uint32_t ino);
static uint32_t min_dir_entry_size(const char* name);
static void update_inode(ext2_fs_t* fs, uint32_t ino, ext2_inode_t* in);
static uint32_t get_or_create_inode_block(ext2_fs_t* fs, ext2_inode_t* in, uint32_t n);
static uint32_t get_inode_block(ext2_fs_t* fs, ext2_inode_t* inode, uint32_t n);
static uint32_t add_directory_entry(ext2_fs_t* fs, const char* name, uint32_t d_ino, uint32_t type);
static list_t* directory_to_entries(ext2_fs_t* fs, uint32_t ino);
static void write_directory_entries(ext2_fs_t* fs, uint32_t ino, list_t* dir_entries);
static dentry_t* make_directory_entry(const char* name, uint32_t ino, uint32_t type);
static void free_directory_entries(list_t* entries);

fs_t* init_ext2(uint8_t* data, uint32_t len) {
    ext2_fs_t* e2fs = kmalloc(sizeof(ext2_fs_t));

    if (len < 1024 + sizeof(superblock_t)) {
        printke("invalid volume: too small to be true");
        return NULL;
    }

    e2fs->device = data;
    e2fs->sb = parse_superblock(e2fs);
    e2fs->group_descriptors = parse_group_descriptors(e2fs);

    if (!e2fs->sb) {
        printke("bad superblock, aborting");
        return NULL;
    }

    e2fs->fs.append = (fs_append_t) ext2_append;
    e2fs->fs.create = (fs_create_t) ext2_create;
    e2fs->fs.rename = (fs_rename_t) ext2_rename;
    e2fs->fs.get_fs_inode = (fs_get_fs_inode_t) ext2_get_fs_inode;
    e2fs->fs.read = (fs_read_t) ext2_read;
    e2fs->fs.readdir = (fs_readdir_t) ext2_readdir;
    e2fs->fs.unlink = (fs_unlink_t) ext2_unlink;
    e2fs->fs.close = (fs_close_t) ext2_close;
    e2fs->fs.stat = (fs_stat_t) ext2_stat;

    e2fs->fs.uid = e2fs->sb->id[0];
    e2fs->fs.root = (folder_inode_t*) ext2_get_fs_inode(e2fs, EXT2_ROOT_INODE);

    return (fs_t*) e2fs;
}

/* Create a file named `name` in the directory pointed to by `parent_inode`.
 * `type` is one of the `DENT_*` constants.
 */
uint32_t ext2_create(ext2_fs_t* fs, const char* name, uint32_t type, uint32_t parent_inode) {
    return add_directory_entry(fs, name, parent_inode, type);
}

/* Deletes the directory entry referencing `ino` in `d_ino`, deleting the inode
 * if it is no longer referenced anywhere.
 */
int32_t ext2_unlink(ext2_fs_t* fs, uint32_t d_ino, uint32_t ino) {
    ext2_inode_t* in = get_inode(fs, ino);

    list_t* entries = directory_to_entries(fs, d_ino);
    list_t* iter;
    dentry_t* ent;

    list_for_each(iter, ent, entries) {
        if (ent->inode == ino) {
            kfree(ent->name);
            kfree(ent);
            list_del(iter);
            break;
        }
    }

    write_directory_entries(fs, d_ino, entries);

    if (--in->hardlinks_count == 0) {
        free_inode(fs, ino);
    } else {
        update_inode(fs, ino, in);
    }

    return 0;
}

/* Move `ino` whose parent directory is `dir_ino`, to the directory `destdir_ino`.
 */
int32_t ext2_rename(ext2_fs_t* fs, uint32_t dir_ino, uint32_t ino, uint32_t destdir_ino) {
    list_t* entries = directory_to_entries(fs, dir_ino);
    list_t* iter;
    dentry_t* ent;

    if (!entries) {
        return -1;
    }

    /* Look for the file to move, remove it from the list */
    list_for_each(iter, ent, entries) {
        if (ent->inode == ino) {
            list_del(iter);
            break;
        }
    }

    /* Not found */
    if (iter == entries) {
        free_directory_entries(entries);
        kfree(entries);

        return -1;
    }

    /* Update the destination dir's list */
    list_t* new_entries = directory_to_entries(fs, destdir_ino);

    if (!new_entries) {
        free_directory_entries(entries);
        kfree(entries);

        return -1;
    }

    list_add(new_entries, make_directory_entry(ent->name, ent->inode, ent->type));

    /* Write changes to disk */
    write_directory_entries(fs, destdir_ino, new_entries);
    write_directory_entries(fs, dir_ino, entries);

    free_directory_entries(entries);
    kfree(entries);
    free_directory_entries(new_entries);
    kfree(new_entries);

    return 0;
}

/* Appends `size` bytes from `data` to the file pointed to by `inode`.
 * Returns the number of bytes written.
 */
uint32_t ext2_append(ext2_fs_t* fs, uint32_t inode, uint8_t* data, uint32_t size) {
    ext2_inode_t* in = get_inode(fs, inode);
    uint8_t* tmp = zalloc(fs->block_size);

    uint32_t start_block = in->size_lower / fs->block_size;
    uint32_t end_block = (in->size_lower + size) / fs->block_size;
    uint32_t offset_start = in->size_lower % fs->block_size;
    uint32_t offset_end = (in->size_lower + size) % fs->block_size;

    if (start_block == end_block) {
        read_inode_block(fs, in, start_block, tmp);
        memcpy(tmp + offset_start, data, size);
        write_inode_block(fs, in, start_block, tmp);
    } else {
        for (uint32_t block = start_block; block < end_block; block++) {
            if (block == start_block) {
                read_inode_block(fs, in, start_block, tmp);
                memcpy(tmp + offset_start, data, fs->block_size - offset_start);
                write_inode_block(fs, in, start_block, tmp);
            } else {
                write_inode_block(fs, in, block, data + (block - start_block)*fs->block_size);
            }
        }

        if (offset_end) {
            memcpy(tmp, data + (end_block - start_block)*fs->block_size - offset_start, offset_end);
            write_inode_block(fs, in, end_block, tmp);
        }
    }

    in->size_lower += size;
    update_inode(fs, inode, in);
    kfree(tmp);
    kfree(in);

    return size;
}

/* Reads at most `size` bytes from `inode`, and returns the number of bytes read.
 * Heavily inspired from https://github.com/klange/toaruos/blob/master/modules/ext2.c
 * I had started similarly but had ended up with much worse code.
 */
uint32_t ext2_read(ext2_fs_t* fs, uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t size) {
    ext2_inode_t* in = get_inode(fs, inode);

    if (!in) {
        return 0;
    }

    uint32_t fsize = in->size_lower;
    uint32_t end;

    if (!size || !fsize || offset >= fsize) {
        kfree(in);
        return 0;
    }

    if (offset + size > fsize) {
        end = fsize;
    } else {
        end = offset + size;
    }

    uint32_t start_block = offset / fs->block_size;
    uint32_t end_block = end / fs->block_size;
    uint32_t start_offset = offset % fs->block_size;
    uint32_t end_offset = end % fs->block_size;
    uint32_t bytes_read = end - offset;

    uint8_t* tmp = kmalloc(fs->block_size);

    if (start_block == end_block) {
        read_inode_block(fs, in, start_block, tmp);
        memcpy(buf, tmp + start_offset, bytes_read);
    } else {
        for (uint32_t block_no = start_block; block_no < end_block; block_no++) {
            if (block_no == start_block) {
                read_inode_block(fs, in, start_block, tmp);
                memcpy(buf, tmp + start_offset, fs->block_size - start_offset);
            } else {
                read_inode_block(fs, in, block_no,
                    buf + (block_no - start_block)*fs->block_size - start_offset);
            }
        }

        if (end_offset) {
            read_inode_block(fs, in, end_block, tmp);
            memcpy(buf + (end_block - start_block)*fs->block_size - start_offset, tmp, end_offset);
        }
    }

    kfree(in);
    kfree(tmp);

    return bytes_read;
}

/* Returns the directory entry at that offset, which must be freed later.
 * `inode` is that of a directory, and `offset` must be at a valid directory
 * entry.
 */
sos_directory_entry_t* ext2_readdir(ext2_fs_t* fs, uint32_t inode, uint32_t offset) {
    ext2_directory_entry_t ent;

    // Read the beginning of the struct to know its size, then read it in full
    uint32_t read = ext2_read(fs, inode, offset, (uint8_t*) &ent, sizeof(ext2_directory_entry_t));

    if (!read) {
        return NULL;
    }

    ext2_directory_entry_t* entry = kmalloc(ent.entry_size);
    read = ext2_read(fs, inode, offset, (uint8_t*) entry, ent.entry_size);

    if (!read) {
        kfree(entry);
        return NULL;
    }

    // This is fine in the specific case of ext2, but not great
    return (sos_directory_entry_t*) entry;
}

inode_t* ext2_get_fs_inode(ext2_fs_t* fs, uint32_t inode) {
    ext2_inode_t* in = get_inode(fs, inode);
    inode_t* fs_in = NULL;

    if (INODE_TYPE(in->type_perms) == INODE_FILE) {
        file_inode_t* fi = kmalloc(sizeof(file_inode_t));
        fi->ino.type = DTYPE_FILE;
        fs_in = (inode_t*) fi;
    } else if (INODE_TYPE(in->type_perms) == INODE_DIR) {
        folder_inode_t* fi = kmalloc(sizeof(folder_inode_t));
        fi->dirty = true;
        fi->subfiles = LIST_HEAD_INIT(fi->subfiles);
        fi->subfolders = LIST_HEAD_INIT(fi->subfolders);
        fi->ino.type = DENT_DIRECTORY;
        fs_in = (inode_t*) fi;
    } else {
        printke("unsupported inode type: %X", INODE_TYPE(in->type_perms));
        return NULL;
    }

    fs_in->inode_no = inode;
    fs_in->size = in->size_lower;
    fs_in->hardlinks = in->hardlinks_count;
    fs_in->fs = (fs_t*) fs;

    kfree(in);

    return fs_in;
}

int32_t ext2_close(ext2_fs_t* fs, uint32_t ino) {
    UNUSED(fs);
    UNUSED(ino);

    return 0;
}

int32_t ext2_stat(ext2_fs_t* fs, uint32_t ino, stat_t* stat) {
    ext2_inode_t* in = get_inode(fs, ino);

    if (!in) {
        return -1;
    }

    stat->st_dev = fs->fs.uid;
    stat->st_ino = ino;
    stat->st_mode = in->type_perms;
    stat->st_nlink = in->hardlinks_count;
    stat->st_size = in->size_lower;

    return 0;
}

/* Utility functions */

/* Reads the content of the given block.
 */
static void read_block(ext2_fs_t* fs, uint32_t block, uint8_t* buf) {
    // TODO: check for max block ?
    memcpy(buf, fs->device + block*fs->block_size, fs->block_size);
}

static void write_block(ext2_fs_t* fs, uint32_t block, uint8_t* buf) {
    memcpy(fs->device + block*fs->block_size, buf, fs->block_size);
}

static void clear_block(ext2_fs_t* fs, uint32_t block) {
    memset(fs->device + block*fs->block_size, 0, fs->block_size);
}

static void write_superblock(ext2_fs_t* fs) {
    write_block(fs, 1, (uint8_t*) fs->sb);
}

static void write_group_descriptor(ext2_fs_t* fs, uint32_t group) {
    write_block(fs, 2, (uint8_t*) &fs->group_descriptors[group]);
}

/* Parses an ext2 superblock from the ext2 partition buffer stored at `data`.
 * Updates `num_block_groups`, and `block_size`.
 * Returns a kmalloc'ed superblock_t struct.
 */
static superblock_t* parse_superblock(ext2_fs_t* fs) {
    superblock_t* sb = kmalloc(1024);
    fs->block_size = 1024;
    read_block(fs, 1, (uint8_t*) sb);

    if (sb->magic != EXT2_MAGIC) {
        printke("invalid signature: %x", sb->magic);
        return NULL;
    }

    fs->num_block_groups = divide_up(sb->blocks_count, sb->blocks_per_group);
    fs->block_size = 1024 << sb->block_size;
    fs->inode_size = sb->inode_size;
    fs->sb = sb;

    return sb;
}

/* Returns a kmalloc'ed array of `num_block_groups` block group descriptors.
 */
static group_descriptor_t* parse_group_descriptors(ext2_fs_t* fs) {
    uint8_t* block = kmalloc(fs->block_size);
    read_block(fs, 2, block);

    uint32_t size = fs->num_block_groups * sizeof(group_descriptor_t);
    group_descriptor_t* bgd = kmalloc(size);

    memcpy((void*) bgd, (void*) block, size);
    kfree(block);

    return bgd;
}

/* Reads the content of the n-th block of the given inode.
 * Assumes that the requested block exists.
 * `n` is relative to the inode, i.e. it's not an absolute block number.
 * For convenience, reading a non-existent block reads zeros.
 */
static void read_inode_block(ext2_fs_t* fs, ext2_inode_t* inode, uint32_t n, uint8_t* buf) {
    uint32_t block = get_inode_block(fs, inode, n);

    if (block) {
        read_block(fs, block, buf);
    } else {
        memset(buf, 0, fs->block_size);
    }
}

/* Writes to the `n`-th block of the given inode.
 * If the block didn't exist, it is created.
 */
static void write_inode_block(ext2_fs_t* fs, ext2_inode_t* inode, uint32_t n, uint8_t* buf) {
    uint32_t block = get_or_create_inode_block(fs, inode, n);

    write_block(fs, block, buf);
}

/* Returns an inode struct from an inode number.
 * This inode can be freed using `kfree`.
 * Note: doesn't check that the inode is valid.
 */
static ext2_inode_t* get_inode(ext2_fs_t* fs, uint32_t inode) {
    if (inode == 0) {
        return NULL;
    }

    // which block group is our inode in?
    uint32_t group = (inode - 1) / fs->sb->inodes_per_group;

    // get the location of the inode table from the group block's descriptor
    uint32_t table_block = fs->group_descriptors[group].inode_table;
    uint32_t block_offset = ((inode - 1) * fs->inode_size) / fs->block_size;
    uint32_t index_in_block = (inode - 1) - block_offset * (fs->block_size / fs->inode_size);

    uint8_t* tmp = kmalloc(fs->block_size);
    read_block(fs, table_block + block_offset, tmp);
    ext2_inode_t* in = kmalloc(fs->inode_size);
    memcpy(in, tmp + index_in_block*fs->inode_size, fs->inode_size);
    kfree(tmp);

    return in;
}

/* Returns a free block number, marking it as used.
 */
static uint32_t allocate_block(ext2_fs_t* fs) {
    /* Exit early */
    if (!fs->sb->free_blocks) {
        return 0;
    }

    /* Find the first non-full block group */
    uint32_t group;
    for (group = 0; group < fs->num_block_groups; group++) {
        if (fs->group_descriptors[group].free_blocs) {
            break;
        }
    }

    /* Go through the bitmap */
    uint32_t bitmap_block = fs->group_descriptors[group].block_bitmap;
    uint32_t block_offset = 0;
    uint8_t* tmp = kmalloc(fs->block_size);

    for (uint32_t i = 0; i < fs->sb->blocks_per_group; i++) {
        if (i % (8 * fs->block_size) == 0) {
            read_block(fs, bitmap_block + block_offset++, tmp);
        }

        uint32_t rel_i = i - (block_offset - 1) * 8 * fs->block_size;
        uint8_t chunk = tmp[rel_i / 8];

        /* Free block found: update disk structures */
        if (!(chunk & (1 << (rel_i % 8)))) {
            tmp[rel_i / 8] = chunk | 1 << (rel_i % 8);
            write_block(fs, bitmap_block + block_offset - 1, tmp);
            kfree(tmp);

            fs->sb->free_blocks--;
            fs->group_descriptors[group].free_blocs--;
            write_superblock(fs);
            write_group_descriptor(fs, group);

            return group * fs->sb->blocks_per_group + i + 1;
        }
    }

    printke("block allocation failed when it shouldn't have");
    abort();

    return 0;
}

static void free_block(ext2_fs_t* fs, uint32_t block) {
    uint32_t group_no = block / fs->sb->blocks_per_group;
    uint32_t bitmap_block = fs->group_descriptors[group_no].block_bitmap;
    uint32_t rel_block = block % fs->sb->blocks_per_group;
    uint32_t block_offset = rel_block / (8 * fs->block_size);
    uint8_t* bitmap = kmalloc(fs->block_size);

    read_block(fs, bitmap_block + block_offset, bitmap);
    bitmap[rel_block / 8] &= ~(1 << (rel_block % 8));
    write_block(fs, bitmap_block + block_offset, bitmap);
    kfree(bitmap);

    fs->sb->free_blocks++;
    fs->group_descriptors[group_no].free_blocs++;
    write_superblock(fs);
    write_group_descriptor(fs, group_no);
}

/* Returns the first free inode number available.
 */
static uint32_t allocate_inode(ext2_fs_t* fs) {
    if (!fs->sb->free_inodes) {
        printke("allocate_inode: no inodes left");
        return 0;
    }

    /* Find a block group with a free inode */
    uint32_t group;
    for (group = 0; group < fs->num_block_groups; group++) {
        if (fs->group_descriptors[group].free_inodes) {
            break;
        }
    }

    /* Go through the bitmap, knowing it's at most a block long */
    uint8_t* tmp = kmalloc(fs->block_size);
    uint32_t bitmap_block = fs->group_descriptors[group].inode_bitmap;
    read_block(fs, bitmap_block, tmp);

    for (uint32_t i = 0; i < fs->sb->inodes_per_group; i++) {
        if (!(tmp[i / 8] & (1 << (i % 8)))) {
            tmp[i / 8] |= 1 << (i % 8);
            write_block(fs, bitmap_block, tmp);
            kfree(tmp);

            fs->group_descriptors[group].free_inodes--;
            fs->sb->free_inodes--;
            write_superblock(fs);
            write_group_descriptor(fs, group);

            // The first bit is inode no. 1
            return i + 1;
        }
    }

    printke("inode allocation failed when it shouldn't have");
    abort();

    return 0;
}

/* Marks an inode as free, along with its allocated blocks.
 */
static void free_inode(ext2_fs_t* fs, uint32_t ino) {
    /* Free the blocks owned by the inode */
    ext2_inode_t* in = get_inode(fs, ino);
    uint32_t num_blocks = divide_up(in->size_lower, fs->block_size);
    kfree(in);

    for (uint32_t iblock = 0; iblock < num_blocks; iblock++) {
        uint32_t rblock = get_inode_block(fs, in, iblock);
        free_block(fs, rblock);
    }

    /* Free the inode itself */
    uint8_t* bitmap = kmalloc(fs->block_size);
    uint32_t group_no = ino / fs->sb->inodes_per_group;
    uint32_t bitmap_block = fs->group_descriptors[group_no].inode_bitmap;
    uint32_t rel_ino = (ino - 1) % fs->sb->inodes_per_group;

    read_block(fs, bitmap_block, bitmap);
    bitmap[rel_ino / 8] &= ~(1 << (rel_ino % 8));
    write_block(fs, bitmap_block, bitmap);
    kfree(bitmap);

    fs->group_descriptors[group_no].free_inodes++;
    fs->sb->free_inodes++;
    write_superblock(fs);
    write_group_descriptor(fs, group_no);
}

/* Returns the minimum size of a directory entry with the given filename.
 */
static uint32_t min_dir_entry_size(const char* name) {
    return align_to(sizeof(ext2_directory_entry_t) + strlen(name), 4);
}

/* Update the inode no. `no` on disk with the content of `in`.
 */
static void update_inode(ext2_fs_t* fs, uint32_t ino, ext2_inode_t* in) {
    uint32_t group = (ino - 1) / fs->sb->inodes_per_group;
    uint32_t table_block = fs->group_descriptors[group].inode_table;
    uint32_t block_offset = ((ino - 1) * fs->inode_size) / fs->block_size;
    uint32_t offset_in_block = ((ino - 1) * fs->inode_size) % fs->block_size;

    uint8_t* tmp = kmalloc(fs->block_size);
    read_block(fs, table_block + block_offset, tmp);
    memcpy(tmp + offset_in_block, in, fs->inode_size);
    write_block(fs, table_block + block_offset, tmp);
    kfree(tmp);
}

/* Returns the nth data block of an inode, creating it if it doesn't exist.
 * Modifies the given inode in place, but not on disk, this is the caller's
 * job.
 * 10$ to anyone who manages to refactor this code to something tolerable.
 */
static uint32_t get_or_create_inode_block(ext2_fs_t* fs, ext2_inode_t* in, uint32_t n) {
    // Number of block pointers in an indirect block
    uint32_t p = fs->block_size / sizeof(uint32_t);
    uint32_t ret = 0;

    if (n < 12) {
        if (!in->dbp[n]) {
            in->dbp[n] = allocate_block(fs);
        }

        ret = in->dbp[n];
    } else if (n < 12 + p) {
        uint32_t* tmp = zalloc(fs->block_size); // Type matters for pointer arithmetic purposes
        uint32_t relblock = n - 12;

        if (!in->sibp) {
            in->sibp = allocate_block(fs);
            write_block(fs, in->sibp, (uint8_t*) tmp);
        }

        read_block(fs, in->sibp, (uint8_t*) tmp);

        if (!tmp[relblock]) {
            tmp[relblock] = allocate_block(fs);
            write_block(fs, in->sibp, (uint8_t*) tmp);
        }

        ret = tmp[relblock];
        kfree(tmp);
    } else if (n < 12 + p + p*p) {
        uint32_t* tmp = zalloc(fs->block_size);
        uint32_t relblock = n - 12 - p;
        uint32_t offset_a = relblock / p;
        uint32_t offset_b = relblock % p;

        if (!in->dibp) {
            in->dibp = allocate_block(fs);
            clear_block(fs, in->dibp);
        }

        read_block(fs, in->dibp, (uint8_t*) tmp);

        if (!tmp[offset_a]) {
            tmp[offset_a] = allocate_block(fs);
            clear_block(fs, tmp[offset_a]);
            write_block(fs, in->dibp, (uint8_t*) tmp);
        }

        uint32_t block_a = tmp[offset_a];
        read_block(fs, tmp[offset_a], (uint8_t*) tmp);

        if (!tmp[offset_b]) {
            tmp[offset_b] = allocate_block(fs);
            write_block(fs, block_a, (uint8_t*) tmp);
        }

        ret = tmp[offset_b];
        kfree(tmp);
    } else if (n < 12 + p + p*p + p*p*p) { // TODO: test this
        uint32_t* tmp = zalloc(fs->block_size);
        uint32_t relblock = n - 12 - p - p*p;
        uint32_t offset_a = relblock / (p*p);
        uint32_t offset_b = relblock % (p*p);
        uint32_t offset_c = relblock % p;

        if (!in->tibp) {
            in->tibp = allocate_block(fs);
            clear_block(fs, in->tibp);
        }

        read_block(fs, in->tibp, (uint8_t*) tmp);

        if (!tmp[offset_a]) {
            tmp[offset_a] = allocate_block(fs);
            clear_block(fs, tmp[offset_a]);
            write_block(fs, in->tibp, (uint8_t*) tmp);
        }

        uint32_t block_a = tmp[offset_a];
        read_block(fs, tmp[offset_a], (uint8_t*) tmp);

        if (!tmp[offset_b]) {
            tmp[offset_b] = allocate_block(fs);
            clear_block(fs, tmp[offset_b]);
            write_block(fs, block_a, (uint8_t*) tmp);
        }

        uint32_t block_b = tmp[offset_b];
        read_block(fs, tmp[offset_b], (uint8_t*) tmp);

        if (!tmp[offset_c]) {
            tmp[offset_c] = allocate_block(fs);
            write_block(fs, block_b, (uint8_t*) tmp);
        }

        ret = tmp[offset_c];
        kfree(tmp);
    } else {
        printke("invalid inode block");
    }

    return ret;
}

/* Returns the nth data block of an inode, or zero if it does not exist.
 * Note: returning zero may simply mean that we're reading a sparse file;
 * such blocks are defined as containing only zeros.
 */
static uint32_t get_inode_block(ext2_fs_t* fs, ext2_inode_t* inode, uint32_t n) {
    // Number of block pointers in an indirect block
    uint32_t p = fs->block_size / sizeof(uint32_t);
    uint32_t ret = 0;

    if (n < 12) {
        ret = inode->dbp[n];
    } else if (n < 12 + p) {
        uint32_t* tmp = kmalloc(fs->block_size);
        uint32_t relblock = n - 12;

        read_block(fs, inode->sibp, (uint8_t*) tmp);
        ret = tmp[relblock];
        kfree(tmp);
    } else if (n < 12 + p + p*p) {
        uint32_t* tmp = kmalloc(fs->block_size);
        uint32_t relblock = n - 12 - p;
        uint32_t offset_a = relblock / p;
        uint32_t offset_b = relblock % p;

        read_block(fs, inode->dibp, (uint8_t*) tmp);
        read_block(fs, tmp[offset_a], (uint8_t*) tmp);
        ret = tmp[offset_b];
        kfree(tmp);
    } else if (n < 12 + p + p*p + p*p*p) { // TODO: test this
        uint32_t* tmp = kmalloc(fs->block_size);
        uint32_t relblock = n - 12 - p - p*p;
        uint32_t offset_a = relblock / (p*p);
        uint32_t offset_b = relblock % (p*p);
        uint32_t offset_c = relblock % p;

        read_block(fs, inode->tibp, (uint8_t*) tmp);
        read_block(fs, tmp[offset_a], (uint8_t*) tmp);
        read_block(fs, tmp[offset_b], (uint8_t*) tmp);
        ret = tmp[offset_c];
        kfree(tmp);
    } else {
        printke("invalid inode block");
    }

    return ret;
}

/* Creates a new entry with name `name` in the directory pointed to by
 * `parent_inode` with type `type`, one of the DTYPE_* constants.
 * Returns the inode of the created file.
 */
static uint32_t add_directory_entry(ext2_fs_t* fs, const char* name, uint32_t d_ino, uint32_t type) {
    ext2_inode_t* d_in = get_inode(fs, d_ino);

    if (!d_in || INODE_TYPE(d_in->type_perms) != INODE_DIR) {
        return 0;
    }

    list_t* entries = directory_to_entries(fs, d_ino);
    uint32_t ino = allocate_inode(fs);
    ext2_inode_t in;

    // Create the new inode
    memset(&in, 0, sizeof(ext2_inode_t));
    in.hardlinks_count = 1;
    list_add(entries, make_directory_entry(name, ino, type));

    // Write the parent directory structure to disk
    write_directory_entries(fs, d_ino, entries);

    // If it's a directory, write its entries to disk
    if (type == DENT_DIRECTORY) {
        in.type_perms = INODE_DIR;
        update_inode(fs, ino, &in);

        list_t dot_list = LIST_HEAD_INIT(dot_list);

        dentry_t* dot = make_directory_entry(".", ino, DENT_DIRECTORY);
        dentry_t* dots = make_directory_entry("..", d_ino, DENT_DIRECTORY);

        list_add(&dot_list, dot);
        list_add(&dot_list, dots);

        write_directory_entries(fs, ino, &dot_list);
        free_directory_entries(&dot_list);
    } else {
        in.type_perms = INODE_FILE;
        update_inode(fs, ino, &in);
    }

    // Free the list of entries, and the rest
    free_directory_entries(entries);
    kfree(entries);
    kfree(d_in);

    return ino;
}

/* Returns a list of `dentry_t` corresponding to the entries in the directory
 * whose inode was given.
 */
static list_t* directory_to_entries(ext2_fs_t* fs, uint32_t ino) {
    list_t* list = kmalloc(sizeof(list_t));
    *list = LIST_HEAD_INIT(*list);

    sos_directory_entry_t* ent = NULL;
    ext2_inode_t* in = get_inode(fs, ino);
    uint32_t offset = 0;

    if (!in) {
        kfree(list);
        return NULL;
    }

    while (offset < in->size_lower) {
        ent = ext2_readdir(fs, ino, offset);

        if (!ent) {
            break;
        }

        dentry_t* tn = kmalloc(sizeof(dentry_t));
        tn->name = strndup(ent->name, ent->name_len_low);
        tn->inode = ent->inode;

        list_add(list, tn);

        offset += ent->entry_size;
        kfree(ent);
    }

    kfree(in);

    return list;
}

/* With `ino` that of a directory, write to it each dentry_t in `dir_entries`.
 */
static void write_directory_entries(ext2_fs_t* fs, uint32_t ino, list_t* dir_entries) {
    ext2_inode_t* in = get_inode(fs, ino);

    if (INODE_TYPE(in->type_perms) != INODE_DIR) {
        return;
    }

    uint8_t* tmp = kmalloc(fs->block_size);
    ext2_directory_entry_t* ent = NULL;
    uint32_t offset = 0;
    uint32_t iblock_no = 0;
    dentry_t* tn;
    list_t* iter;

    list_for_each(iter, tn, dir_entries) {
        /* Switch to the next iblock, resize the previous entry to fill the block */
        if (offset + min_dir_entry_size(tn->name) >= fs->block_size) {
            printk("write_directory_entries: more than one block");
            offset -= ent->entry_size;
            ent->entry_size = fs->block_size - offset;
            offset = 0;
            write_inode_block(fs, in, iblock_no++, tmp);
        }

        ent = (ext2_directory_entry_t*) &tmp[offset];
        bool last = list_last_entry(dir_entries, dentry_t) == tn;

        ent->inode = tn->inode;
        ent->name_len_low = strlen(tn->name);
        ent->type = tn->type;
        ent->entry_size = last ? fs->block_size - offset : min_dir_entry_size(tn->name);
        strncpy(ent->name, tn->name, strlen(tn->name));

        offset += ent->entry_size;
    }

    write_inode_block(fs, in, iblock_no, tmp);

    in->size_lower = fs->block_size * iblock_no + offset;
    update_inode(fs, ino, in);

    kfree(tmp);
    kfree(in);
}

static dentry_t* make_directory_entry(const char* name, uint32_t ino, uint32_t type) {
    dentry_t* tn = kmalloc(sizeof(dentry_t));

    tn->inode = ino;
    tn->name = strdup(name);
    tn->type = type == DENT_DIRECTORY ? DTYPE_DIR : DTYPE_FILE;

    return tn;
}

/* Does not free the list itself.
 */
static void free_directory_entries(list_t* entries) {
    while (!list_empty(entries)) {
        dentry_t* tn = list_first_entry(entries, dentry_t);
        kfree(tn->name);
        kfree(tn);
        list_del(list_first(entries));
    }
}