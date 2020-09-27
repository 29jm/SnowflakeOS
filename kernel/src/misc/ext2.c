#include <kernel/ext2.h>
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
	// >= 1.x
	uint32_t first_available_block;
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
} inode_t;

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

#define U16_BIGENDIAN(n) ((n & 0xFF >> 8) | (n << 8))
#define AS_U16(data) (*(uint16_t*) data)

static uint8_t* device;
static uint32_t num_block_groups;
static uint32_t block_size;
static superblock_t* superblock;
static group_descriptor_t* group_descriptors;

superblock_t* parse_superblock(uint8_t* data) {
	superblock_t* sb = kmalloc(1024);
	memcpy((void*) sb, (void*) data, 1024);

	if (sb->magic != 0xEF53) {
		printf("[EXT2] Invalid signature: %x, %x, %x, %x\n", data[56], data[57]);
		return NULL;
	} else {
		printf("[EXT2] Valid volume signature\n");
	}

	num_block_groups = divide_up(sb->blocks_count, sb->blocks_per_group);
	block_size = 1024 << sb->block_size;

	if (num_block_groups != divide_up(sb->inodes_count, sb->inodes_per_group)) {
		printf("[EXT2] incoherent block groups count found\n");
		return NULL;
	}

	printf("[EXT2] Block count: %d\n", sb->blocks_count);
	printf("[EXT2] Blocks per group: %d\n", sb->blocks_per_group);
	printf("[EXT2] inode count: %d\n", sb->inodes_count);
	printf("[EXT2] inodes per group: %d\n", sb->inodes_per_group);
	printf("[EXT2] Block groups: %d\n", num_block_groups);
	printf("[EXT2] version: %d-%d\n", sb->version_major, sb->version_minor);
	printf("[EXT2] inode size: %d\n", sb->inode_size);
	printf("[EXT2] block size: %d\n", block_size);

	return sb;
}

group_descriptor_t* parse_group_descriptors(uint8_t* data) {
	uint32_t size = num_block_groups*sizeof(group_descriptor_t);
	group_descriptor_t* bgd = kmalloc(size);
	memcpy((void*) bgd, (void*) data, size);

	return bgd;
}

inode_t* ext2_get_inode(uint32_t inode) {
	if (inode == 0) {
		return NULL;
	}

	// which block group is our inode in?
	uint32_t group = (inode - 1) / superblock->inodes_per_group;

	// get the location of the inode table from the group block's descriptor
	uint32_t table_block = group_descriptors[group].inode_table;
	uint32_t block_offset = ((inode - 1) * superblock->inode_size) / block_size;
	uint32_t index_in_block = (inode - 1) - block_offset * (block_size / superblock->inode_size);
	uintptr_t offset = (table_block + block_offset)*block_size + index_in_block*superblock->inode_size;

	return (inode_t*) (device + offset);
}

/* Returns true if the first n characters of a and b are equal, false otherwise.
 */
bool equal_n(const char* a, const char* b, uint32_t n) {
	for (uint32_t i = 0; i < n; i++) {
		if (a[i] != b[i]) {
			return false;
		}
	}

	return true;
}

/* Returns the inode corresponding to the file at `path`.
 * On error, returns 0, the invalid inode.
 */
uint32_t ext2_open(const char* path) {
	char* p = (char*) path;
	uint32_t n = strlen(p);

	if (p[0] != '/') {
		return 0;
	}

	if (n == 1 && p[0] == '/') {
		return 2;
	}

	p++;

	inode_t* in = ext2_get_inode(EXT2_ROOT_INODE);
	ext2_directory_entry_t* entry = (ext2_directory_entry_t*) (device + block_size*in->dbp[0]);

	while (p < path + n && entry->type != 0) { // TODO define
		char* c = strchrnul(p, '/');
		uint32_t len = c ? (uint32_t) (c - p) : strlen(p);

		if (equal_n(p, entry->name, len)) {
			p += len + 1;

			if (p >= path + n) {
				return entry->inode;
			} else {
				in = ext2_get_inode(entry->inode);
				entry = (ext2_directory_entry_t*) (device + block_size*in->dbp[0]);
			}
		}

		entry = (ext2_directory_entry_t*) ((uintptr_t) entry + entry->entry_size);
	}

	return 0;
}

/* Reads the content of the given block.
 */
void ext2_read_block(uint32_t block, uint8_t* buf) {
	// TODO: check for max block ?
	memcpy(buf, device + block*block_size, block_size);
}

/* Reads the content of n-th block of the given inode.
 */
void ext2_read_inode_block(inode_t* inode, uint32_t n, uint8_t* buf) {
	if (n >= 12) {
		printf("[ext2] unimplemented\n");
		return;
	}

	ext2_read_block(inode->dbp[n], buf);
}

/* Reads at most `size` bytes from `inode`, and returns the number of bytes read.
 * Heavily inspired from https://github.com/klange/toaruos/blob/master/modules/ext2.c
 * I had started similarly but had ended up with much worse code.
 */
uint32_t ext2_read(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t size) {
	inode_t* in = ext2_get_inode(inode);

	if (!in) {
		return 0;
	}

	uint32_t fsize = in->size_lower;
	uint32_t end;

	if (!size || !fsize || offset >= fsize) {
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
				ext2_read_inode_block(in, block_no, tmp);
				memcpy(buf + start_offset + (block_no - start_block)*block_size - start_offset, tmp, block_size);
			}
		}

		if (end_offset) {
			ext2_read_inode_block(in, end_block, tmp);
			memcpy(buf + (end_block - start_block)*block_size - start_offset, tmp, end_offset);
		}
	}

	kfree(tmp);

	return bytes_read;
}

/* `inode` is that of a directory, and `offset` must be at a valid directory
 * entry.
 * Returns the directory entry at that offset.
 */
ext2_directory_entry_t* ext2_readdir(uint32_t inode, uint32_t offset) {
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

	return entry;
}

void init_ext2(uint8_t* data, uint32_t len) {
	if (len < 1024 + sizeof(superblock_t)) {
		printf("[EXT2] Invalid volume: too small to be true\n");
		return;
	}

	device = data;
	superblock = parse_superblock(&data[1024]);

	if (!superblock) {
		printf("[EXT2] aborting\n");
		return;
	}

	group_descriptors = parse_group_descriptors(&data[2048]);
}