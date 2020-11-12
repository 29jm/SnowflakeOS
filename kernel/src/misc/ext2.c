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

typedef struct {
    uint32_t inode;
    uint16_t entry_size;
    uint8_t name_len_low;
    uint8_t type;
    char name[];
} ext2_directory_entry_t;

typedef struct dentry_t {
    char* name;
    uint32_t inode;
} dentry_t;

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

#define U16_BIGENDIAN(n) ((n & 0xFF >> 8) | (n << 8))
#define AS_U16(data) (*(uint16_t*) data)

static uint8_t* device;
static uint32_t num_block_groups;
static uint32_t block_size;
static uint32_t inode_size;
static superblock_t* superblock;
static group_descriptor_t* group_descriptors;

uint32_t ext2_get_inode_block(ext2_inode_t* in, uint32_t n);
uint32_t ext2_get_or_create_inode_block(ext2_inode_t* in, uint32_t n);

/* Reads the content of the given block.
 */
void ext2_read_block(uint32_t block, uint8_t* buf) {
    // TODO: check for max block ?
    memcpy(buf, device + block*block_size, block_size);
}

void ext2_write_block(uint32_t block, uint8_t* buf) {
    memcpy(device + block*block_size, buf, block_size);
}

void ext2_clear_block(uint32_t block) {
    memset(device + block*block_size, 0, block_size);
}

void ext2_write_superblock() {
    ext2_write_block(1, (uint8_t*) superblock);
}

void ext2_write_group_descriptor(uint32_t group) {
    ext2_write_block(2, (uint8_t*) &group_descriptors[group]);
}

/* Parses an ext2 superblock from the ext2 partition buffer stored at `data`.
 * Updates `num_block_groups`, and `block_size`.
 * Returns a kmalloc'ed superblock_t struct.
 */
superblock_t* parse_superblock() {
    superblock_t* sb = kmalloc(1024);
    block_size = 1024;
    ext2_read_block(1, (uint8_t*) sb);

    if (sb->magic != EXT2_MAGIC) {
        printke("invalid signature: %x", sb->magic);
        return NULL;
    }

    num_block_groups = divide_up(sb->blocks_count, sb->blocks_per_group);
    block_size = 1024 << sb->block_size;
    inode_size = sb->inode_size;

    return sb;
}

/* Returns a kmalloc'ed array of `num_block_groups` block group descriptors.
 */
group_descriptor_t* parse_group_descriptors() {
    uint8_t* block = kmalloc(block_size);
    ext2_read_block(2, block);

    uint32_t size = num_block_groups * sizeof(group_descriptor_t);
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
void ext2_read_inode_block(ext2_inode_t* inode, uint32_t n, uint8_t* buf) {
    uint32_t block = ext2_get_inode_block(inode, n);

    if (block) {
        ext2_read_block(block, buf);
    } else {
        memset(buf, 0, block_size);
    }
}

/* Writes to the `n`-th block of the given inode.
 * If the block didn't exist, it is created.
 */
void ext2_write_inode_block(ext2_inode_t* inode, uint32_t n, uint8_t* buf) {
    uint32_t block = ext2_get_or_create_inode_block(inode, n);

    ext2_write_block(block, buf);
}

/* Returns an inode struct from an inode number.
 * This inode can be freed using `kfree`.
 * Note: doesn't check that the inode is valid.
 */
ext2_inode_t* ext2_get_inode(uint32_t inode) {
    if (inode == 0) {
        return NULL;
    }

    // which block group is our inode in?
    uint32_t group = (inode - 1) / superblock->inodes_per_group;

    // get the location of the inode table from the group block's descriptor
    uint32_t table_block = group_descriptors[group].inode_table;
    uint32_t block_offset = ((inode - 1) * superblock->inode_size) / block_size;
    uint32_t index_in_block = (inode - 1) - block_offset * (block_size / superblock->inode_size);

    uint8_t* tmp = kmalloc(block_size);
    ext2_read_block(table_block + block_offset, tmp);
    ext2_inode_t* in = kmalloc(superblock->inode_size);
    memcpy(in, tmp + index_in_block*superblock->inode_size, superblock->inode_size);
    kfree(tmp);

    return in;
}

/* Returns a free block number, marking it as used.
 */
uint32_t ext2_allocate_block() {
    uint8_t* tmp = kmalloc(block_size);
    uint32_t j = 0;
    uint32_t bitmap = 0;
    ext2_read_block(bitmap++, tmp);

    while (j < superblock->blocks_per_group*num_block_groups) {
        uint32_t group = j / superblock->blocks_per_group;
        if (j % superblock->blocks_per_group == 0) {
            bitmap = group_descriptors[group].block_bitmap;
        }

        if (j % (8*block_size) == 0) {
            ext2_read_block(bitmap, tmp);
            bitmap++;
        }

        if (!(tmp[j / 8] & (1 << (j % 8)))) {
            tmp[j / 8] |= 1 << (j % 8);
            ext2_write_block(bitmap - 1, tmp);
            kfree(tmp);

            superblock->free_blocks--;
            group_descriptors[group].free_blocs--;

            ext2_write_superblock();
            ext2_write_group_descriptor(group);

            return j;
        }

        j++;
    }

    kfree(tmp);

    return 0;
}

void ext2_free_block(uint32_t block) {
    uint32_t group_no = block / superblock->blocks_per_group;
    uint32_t bitmap_block = group_descriptors[group_no].block_bitmap;
    uint32_t rel_block = block % superblock->blocks_per_group;
    uint32_t block_offset = rel_block / (8 * block_size);
    uint8_t* bitmap = kmalloc(block_size);

    ext2_read_block(bitmap_block + block_offset, bitmap);
    bitmap[rel_block / 8] &= ~(1 << (rel_block % 8));
    ext2_write_block(bitmap_block + block_offset, bitmap);
    kfree(bitmap);

    superblock->free_blocks++;
    group_descriptors[group_no].free_blocs++;
    ext2_write_superblock();
    ext2_write_group_descriptor(group_no);
}

/* Returns the first free inode number available.
 */
uint32_t ext2_allocate_inode() {
    uint8_t* bitmap = kmalloc(block_size);

    for (uint32_t group = 0; group < num_block_groups; group++) {
        uint32_t bitmap_block = group_descriptors[group].inode_bitmap;

        ext2_read_block(bitmap_block, bitmap);

        for (uint32_t i = 0; i < 8*block_size; i++) {
            if (!(bitmap[i / 8] & (1 << (i % 8)))) {
                bitmap[i / 8] |= 1 << (i % 8);
                ext2_write_block(bitmap_block, bitmap);
                kfree(bitmap);

                group_descriptors[group].free_inodes--;
                superblock->free_inodes--;
                ext2_write_superblock();
                ext2_write_group_descriptor(group);

                // The first bit is inode no. 1
                return i + 1;
            }
        }

    }

    kfree(bitmap);
    printke("Failed to allocate inode");

    return 0;
}

/* Marks an inode as free, along with its allocated blocks.
 */
void ext2_free_inode(uint32_t ino) {
    /* Free the blocks owned by the inode */
    ext2_inode_t* in = ext2_get_inode(ino);
    uint32_t num_blocks = divide_up(in->size_lower, block_size);
    kfree(in);

    for (uint32_t iblock = 0; iblock < num_blocks; iblock++) {
        uint32_t rblock = ext2_get_inode_block(in, iblock);
        ext2_free_block(rblock);
    }

    /* Free the inode itself */
    uint8_t* bitmap = kmalloc(block_size);
    uint32_t group_no = ino / superblock->inodes_per_group;
    uint32_t bitmap_block = group_descriptors[group_no].inode_bitmap;
    uint32_t rel_ino = (ino - 1) % superblock->inodes_per_group;

    ext2_read_block(bitmap_block, bitmap);
    bitmap[rel_ino / 8] &= ~(1 << (rel_ino % 8));
    ext2_write_block(bitmap_block, bitmap);
    kfree(bitmap);

    group_descriptors[group_no].free_inodes++;
    superblock->free_inodes++;
    ext2_write_superblock();
    ext2_write_group_descriptor(group_no);
}

/* Returns the minimum size of a directory entry with the given filename.
 */
uint32_t ext2_min_dir_entry_size(const char* name) {
    return align_to(sizeof(ext2_directory_entry_t) + strlen(name), 4);
}

/* Update the inode no. `no` on disk with the content of `in`.
 */
void ext2_update_inode(uint32_t no, ext2_inode_t* in) {
    uint32_t group = (no - 1) / superblock->inodes_per_group;
    uint32_t table_block = group_descriptors[group].inode_table;
    uint32_t block_offset = ((no - 1) * inode_size) / block_size;
    uint32_t offset_in_block = ((no - 1) * inode_size) % block_size;

    uint8_t* tmp = kmalloc(block_size);
    ext2_read_block(table_block + block_offset, tmp);
    memcpy(tmp + offset_in_block, in, inode_size);
    ext2_write_block(table_block + block_offset, tmp);
    kfree(tmp);
}

/* Returns the nth data block of an inode, creating it if it doesn't exist.
 * Modifies the given inode in place, but not on disk, this is the caller's
 * job.
 * 10$ to anyone who manages to refactor this code to something tolerable.
 */
uint32_t ext2_get_or_create_inode_block(ext2_inode_t* in, uint32_t n) {
    // Number of block pointers in an indirect block
    uint32_t p = block_size / sizeof(uint32_t);
    // Type matters for pointer arithmetic purposes
    static uint32_t* tmp = NULL;

    if (!tmp) {
        tmp = zalloc(block_size);
    }

    if (n < 12) {
        if (!in->dbp[n]) {
            in->dbp[n] = ext2_allocate_block();
        }

        return in->dbp[n];
    } else if (n < 12 + p) {
        uint32_t relblock = n - 12;

        if (!in->sibp) {
            in->sibp = ext2_allocate_block();
            ext2_write_block(in->sibp, (uint8_t*) tmp);
        }

        ext2_read_block(in->sibp, (uint8_t*) tmp);

        if (!tmp[relblock]) {
            tmp[relblock] = ext2_allocate_block();
            ext2_write_block(in->sibp, (uint8_t*) tmp);
        }

        return tmp[relblock];
    } else if (n < 12 + p + p*p) {
        uint32_t relblock = n - 12 - p;
        uint32_t offset_a = relblock / p;
        uint32_t offset_b = relblock % p;

        if (!in->dibp) {
            in->dibp = ext2_allocate_block();
            ext2_clear_block(in->dibp);
        }

        ext2_read_block(in->dibp, (uint8_t*) tmp);

        if (!tmp[offset_a]) {
            tmp[offset_a] = ext2_allocate_block();
            ext2_clear_block(tmp[offset_a]);
            ext2_write_block(in->dibp, (uint8_t*) tmp);
        }

        uint32_t block_a = tmp[offset_a];
        ext2_read_block(tmp[offset_a], (uint8_t*) tmp);

        if (!tmp[offset_b]) {
            tmp[offset_b] = ext2_allocate_block();
            ext2_write_block(block_a, (uint8_t*) tmp);
        }

        return tmp[offset_b];
    } else if (n < 12 + p + p*p + p*p*p) { // TODO: test this
        uint32_t relblock = n - 12 - p - p*p;
        uint32_t offset_a = relblock / (p*p);
        uint32_t offset_b = relblock % (p*p);
        uint32_t offset_c = relblock % p;

        if (!in->tibp) {
            in->tibp = ext2_allocate_block();
            ext2_clear_block(in->tibp);
        }

        ext2_read_block(in->tibp, (uint8_t*) tmp);

        if (!tmp[offset_a]) {
            tmp[offset_a] = ext2_allocate_block();
            ext2_clear_block(tmp[offset_a]);
            ext2_write_block(in->tibp, (uint8_t*) tmp);
        }

        uint32_t block_a = tmp[offset_a];
        ext2_read_block(tmp[offset_a], (uint8_t*) tmp);

        if (!tmp[offset_b]) {
            tmp[offset_b] = ext2_allocate_block();
            ext2_clear_block(tmp[offset_b]);
            ext2_write_block(block_a, (uint8_t*) tmp);
        }

        uint32_t block_b = tmp[offset_b];
        ext2_read_block(tmp[offset_b], (uint8_t*) tmp);

        if (!tmp[offset_c]) {
            tmp[offset_c] = ext2_allocate_block();
            ext2_write_block(block_b, (uint8_t*) tmp);
        }

        return tmp[offset_c];
    } else {
        printke("invalid inode block");
    }

    return 0;
}

uint32_t ext2_get_inode_block(ext2_inode_t* inode, uint32_t n) {
    // Number of block pointers in an indirect block
    uint32_t p = block_size / sizeof(uint32_t);
    static uint32_t* tmp = NULL;

    if (!tmp) {
        tmp = kmalloc(block_size);
    }

    if (n < 12) {
        return inode->dbp[n];
    } else if (n < 12 + p) {
        uint32_t relblock = n - 12;

        ext2_read_block(inode->sibp, (uint8_t*) tmp);

        return tmp[relblock];
    } else if (n < 12 + p + p*p) {
        uint32_t relblock = n - 12 - p;
        uint32_t offset_a = relblock / p;
        uint32_t offset_b = relblock % p;

        ext2_read_block(inode->dibp, (uint8_t*) tmp);
        ext2_read_block(tmp[offset_a], (uint8_t*) tmp);

        return tmp[offset_b];
    } else if (n < 12 + p + p*p + p*p*p) { // TODO: test this
        uint32_t relblock = n - 12 - p - p*p;
        uint32_t offset_a = relblock / (p*p);
        uint32_t offset_b = relblock % (p*p);
        uint32_t offset_c = relblock % p;

        ext2_read_block(inode->tibp, (uint8_t*) tmp);
        ext2_read_block(tmp[offset_a], (uint8_t*) tmp);
        ext2_read_block(tmp[offset_b], (uint8_t*) tmp);

        return tmp[offset_c];
    }

    printke("invalid inode block");

    return 0;
}

/* Returns a list of `dentry_t` corresponding to the entries in the directory
 * whose inode was given.
 */
list_t* ext2_dir_to_list(uint32_t ino) {
    list_t* list = kmalloc(sizeof(list_t));
    *list = LIST_HEAD_INIT(*list);

    sos_directory_entry_t* ent = NULL;
    uint32_t offset = 0;

    do {
        ent = ext2_readdir(ino, offset);

        if (!ent) {
            break;
        }

        dentry_t* tn = kmalloc(sizeof(dentry_t));
        tn->name = strndup(ent->name, ent->name_len_low);
        tn->inode = ent->inode;

        list_add(list, tn);

        offset += ent->entry_size;
        kfree(ent);
    } while (offset < block_size);

    return list;
}

/* With `ino` that of a directory, write to it each dentry_t in `dir_entries`.
 * TODO: make it work for directories spanning more than one block.
 */
void ext2_write_dir(uint32_t ino, list_t* dir_entries) {
    ext2_inode_t* in = ext2_get_inode(ino);

    if (INODE_TYPE(in->type_perms) != INODE_DIR) {
        return;
    }

    uint8_t* tmp = kmalloc(block_size);
    uint32_t offset = 0;
    dentry_t* tn;
    list_for_each_entry(tn, dir_entries) {
        ext2_inode_t* ent_in = ext2_get_inode(tn->inode);
        ext2_directory_entry_t* ent = (ext2_directory_entry_t*) &tmp[offset];
        bool last = list_last_entry(dir_entries, dentry_t) == tn;

        ent->inode = tn->inode;
        ent->name_len_low = strlen(tn->name);
        ent->type = INODE_TYPE(ent_in->type_perms) == INODE_DIR ? DTYPE_DIR : DTYPE_FILE;
        ent->entry_size = last ? block_size - offset : ext2_min_dir_entry_size(tn->name);
        strncpy(ent->name, tn->name, strlen(tn->name));

        offset += ent->entry_size;
        kfree(ent_in);
    }

    ext2_write_inode_block(in, 0, tmp);
    kfree(tmp);
    kfree(in);
}

dentry_t* ext2_make_dir_entry(const char* name, uint32_t ino) {
    dentry_t* tn = kmalloc(sizeof(dentry_t));

    tn->inode = ino;
    tn->name = strdup(name);

    return tn;
}

/* Does not free the list itself.
 */
void ext2_free_dir_entries(list_t* entries) {
    while (!list_empty(entries)) {
        dentry_t* tn = list_first_entry(entries, dentry_t);
        kfree(tn->name);
        kfree(tn);
        list_del(list_first(entries));
    }
}

/* Creates a new entry with name `name` in the directory pointed to by
 * `parent_inode` with type `type`, one of the DTYPE_* constants.
 * Returns the inode of the created file.
 */
uint32_t ext2_add_dentry(const char* name, uint32_t d_ino, uint32_t type) {
    ext2_inode_t* d_in = ext2_get_inode(d_ino);

    if (!d_in || INODE_TYPE(d_in->type_perms) != INODE_DIR) {
        return 0;
    }

    list_t* entries = ext2_dir_to_list(d_ino);
    uint32_t ino = ext2_allocate_inode();
    ext2_inode_t in;

    // Create the new inode
    memset(&in, 0, sizeof(ext2_inode_t));
    in.hardlinks_count = 1;
    list_add(entries, ext2_make_dir_entry(name, ino));

    // Write the parent directory structure to disk
    ext2_write_dir(d_ino, entries);

    // If it's a directory, write its entries to disk
    if (type == DTYPE_DIR) {
        in.type_perms = INODE_DIR;
        in.dbp[0] = ext2_allocate_block();
        in.size_lower = block_size;
        ext2_update_inode(ino, &in);

        list_t dot_list = LIST_HEAD_INIT(dot_list);

        dentry_t* dot = ext2_make_dir_entry(".", ino);
        dentry_t* dots = ext2_make_dir_entry("..", d_ino);

        list_add(&dot_list, dot);
        list_add(&dot_list, dots);

        ext2_write_dir(ino, &dot_list);
        ext2_free_dir_entries(&dot_list);
    } else {
        in.type_perms = INODE_FILE;
        ext2_update_inode(ino, &in);
    }

    // Free the list of entries, and the rest
    ext2_free_dir_entries(entries);
    kfree(entries);
    kfree(d_in);

    return ino;
}

/* Create a file named `name` in the directory pointed to by `parent_inode`.
 * `type` is one of the `DENT_*` constants.
 */
uint32_t ext2_create(const char* name, uint32_t type, uint32_t parent_inode) {
    uint32_t inode = ext2_add_dentry(name, parent_inode, type);
    ext2_inode_t* in = ext2_get_inode(inode);

    in->type_perms = type == DENT_DIRECTORY ? INODE_DIR : INODE_FILE;

    ext2_update_inode(inode, in);
    kfree(in);

    return inode;
}

/* Deletes the directory entry referencing `ino` in `d_ino`, deleting the inode
 * if it is no longer referenced anywhere.
 */
int32_t ext2_unlink(uint32_t d_ino, uint32_t ino) {
    ext2_inode_t* in = ext2_get_inode(ino);

    list_t* entries = ext2_dir_to_list(d_ino);
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

    ext2_write_dir(d_ino, entries);

    if (--in->hardlinks_count == 0) {
        ext2_free_inode(ino);
    } else {
        ext2_update_inode(ino, in);
    }

    return 0;
}

/* Appends `size` bytes from `data` to the file pointed to by `inode`.
 * Returns the number of bytes written.
 */
uint32_t ext2_append(uint32_t inode, uint8_t* data, uint32_t size) {
    ext2_inode_t* in = ext2_get_inode(inode);
    uint8_t* tmp = zalloc(block_size);

    uint32_t start_block = in->size_lower / block_size;
    uint32_t end_block = (in->size_lower + size) / block_size;
    uint32_t offset_start = in->size_lower % block_size;
    uint32_t offset_end = (in->size_lower + size) % block_size;

    if (start_block == end_block) {
        ext2_read_inode_block(in, start_block, tmp);
        memcpy(tmp + offset_start, data, size);
        ext2_write_inode_block(in, start_block, tmp);
    } else {
        for (uint32_t block = start_block; block < end_block; block++) {
            if (block == start_block) {
                ext2_read_inode_block(in, start_block, tmp);
                memcpy(tmp + offset_start, data, block_size - offset_start);
                ext2_write_inode_block(in, start_block, tmp);
            } else {
                ext2_write_inode_block(in, block, data + (block - start_block)*block_size);
            }
        }

        if (offset_end) {
            memcpy(tmp, data + (end_block - start_block)*block_size - offset_start, offset_end);
            ext2_write_inode_block(in, end_block, tmp);
        }
    }

    in->size_lower += size;
    ext2_update_inode(inode, in);
    kfree(tmp);

    return size;
}

/* Reads at most `size` bytes from `inode`, and returns the number of bytes read.
 * Heavily inspired from https://github.com/klange/toaruos/blob/master/modules/ext2.c
 * I had started similarly but had ended up with much worse code.
 */
uint32_t ext2_read(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t size) {
    ext2_inode_t* in = ext2_get_inode(inode);

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

    uint32_t start_block = offset / block_size;
    uint32_t end_block = end / block_size;
    uint32_t start_offset = offset % block_size;
    uint32_t end_offset = end % block_size;
    uint32_t bytes_read = end - offset;

    uint8_t* tmp = kmalloc(block_size);

    if (start_block == end_block) {
        ext2_read_inode_block(in, start_block, tmp);
        memcpy(buf, tmp + start_offset, bytes_read);
    } else {
        for (uint32_t block_no = start_block; block_no < end_block; block_no++) {
            if (block_no == start_block) {
                ext2_read_inode_block(in, start_block, tmp);
                memcpy(buf, tmp + start_offset, block_size - start_offset);
            } else {
                ext2_read_inode_block(in, block_no,
                    buf + (block_no - start_block)*block_size);
            }
        }

        if (end_offset) {
            ext2_read_inode_block(in, end_block, tmp);
            memcpy(buf + (end_block - start_block)*block_size - start_offset, tmp, end_offset);
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
sos_directory_entry_t* ext2_readdir(uint32_t inode, uint32_t offset) {
    ext2_directory_entry_t ent;

    // Read the beginning of the struct to know its size, then read it in full
    uint32_t read = ext2_read(inode, offset, (uint8_t*) &ent, sizeof(ext2_directory_entry_t));

    if (!read) {
        return NULL;
    }

    ext2_directory_entry_t* entry = kmalloc(ent.entry_size);
    read = ext2_read(inode, offset, (uint8_t*) entry, ent.entry_size);

    if (!read) {
        kfree(entry);
        return NULL;
    }

    // This is fine in the specific case of ext2, but not great
    return (sos_directory_entry_t*) entry;
}

inode_t* ext2_get_fs_inode(uint32_t inode) {
    ext2_inode_t* in = ext2_get_inode(inode);
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
        fi->ino.type = DTYPE_DIR;
        fs_in = (inode_t*) fi;
    } else {
        printke("unsupported inode type: %X", INODE_TYPE(in->type_perms));
    }

    fs_in->inode_no = inode;
    fs_in->size = in->size_lower;
    fs_in->hardlinks = in->hardlinks_count;

    kfree(in);

    return fs_in;
}

void init_ext2(uint8_t* data, uint32_t len) {
    if (len < 1024 + sizeof(superblock_t)) {
        printke("invalid volume: too small to be true");
        return;
    }

    device = data;
    superblock = parse_superblock();

    if (!superblock) {
        printke("bad superblock, aborting");
        return;
    }

    group_descriptors = parse_group_descriptors();

    printk("initialized volume of size %d KiB", len >> 10);
}