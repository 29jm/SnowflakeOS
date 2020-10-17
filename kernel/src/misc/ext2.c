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

uint32_t ext2_get_inode_block(inode_t* in, uint32_t n);
uint32_t ext2_get_or_create_inode_block(inode_t* in, uint32_t n);

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
		printf("[EXT2] Invalid signature: %x\n", sb->magic);
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
void ext2_read_inode_block(inode_t* inode, uint32_t n, uint8_t* buf) {
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
void ext2_write_inode_block(inode_t* inode, uint32_t n, uint8_t* buf) {
	uint32_t block = ext2_get_or_create_inode_block(inode, n);

	ext2_write_block(block, buf);
}

/* Returns an inode struct from an inode number.
 * This inode can be freed using `kfree`.
 * Expects `parse_superblock` to have been called.
 */
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

	uint8_t* tmp = kmalloc(block_size);
	ext2_read_block(table_block + block_offset, tmp);
	inode_t* in = kmalloc(superblock->inode_size);
	memcpy(in, tmp + index_in_block*superblock->inode_size, superblock->inode_size);
	kfree(tmp);

	return in;
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
	uint8_t* block = kmalloc(block_size);
	ext2_read_block(in->dbp[0], block);

	ext2_directory_entry_t* entry = (ext2_directory_entry_t*) block;

	while (p < path + n && entry->type != DTYPE_INVALID) {
		char* c = strchrnul(p, '/');
		uint32_t len = c ? (uint32_t) (c - p) : strlen(p);

		if (!memcmp(p, entry->name, len)) {
			p += len + 1;

			if (p >= path + n) {
				kfree(block);
				kfree(in);

				return entry->inode;
			} else {
				kfree(in);
				in = ext2_get_inode(entry->inode);

				ext2_read_block(in->dbp[0], block);
				entry = (ext2_directory_entry_t*) block;
			}
		}

		entry = (ext2_directory_entry_t*) ((uintptr_t) entry + entry->entry_size);
	}

	kfree(in);
	kfree(block);

	return 0;
}

/* Returns the number of a free block, marking it as used.
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
	printf("[ext2] failed to allocate inode\n");

	return 0;
}

/* Returns the minimum size of a directory entry with the given filename.
 */
uint32_t ext2_min_dir_entry_size(const char* name) {
	return sizeof(ext2_directory_entry_t) + strlen(name);
}

/* Update the inode no. `no` on disk with the content of `in`.
 */
void ext2_update_inode(uint32_t no, inode_t* in) {
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
uint32_t ext2_get_or_create_inode_block(inode_t* in, uint32_t n) {
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
		printf("[EXT2] Invalid inode block\n");
	}

	return 0;
}

uint32_t ext2_get_inode_block(inode_t* inode, uint32_t n) {
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

	printf("[EXT2] Invalid inode block\n");

	return 0;
}

/* Creates a new entry with name `name` in the directory pointed to by
 * `parent_inode` with type `type`, one of the DTYPE_* constants.
 * Returns the inode of the created file.
 */
uint32_t ext2_add_directory_entry(const char* name, uint32_t parent_inode, uint32_t type) {
	inode_t* in = ext2_get_inode(parent_inode);

	if (!in) {
		printf("[ext2] add_entry: parent does not exist\n");
		return 0;
	}

	if (INODE_TYPE(in->type_perms) != INODE_DIR) {
		printf("[ext2] add_entry: parent is not a directory\n");
		return 0;
	}

	uint8_t* block = kmalloc(block_size);
	ext2_read_inode_block(in, 0, block);
	ext2_directory_entry_t* e;
	uint32_t offset = 0;

	/* Look for the last entry and resize it */
	do {
		e = ext2_readdir(parent_inode, offset);

		// This is the last entry, resize it to make room for one more
		// TODO: don't assume this block isn't full
		if (offset + e->entry_size == block_size) {
			uint32_t new_size = sizeof(ext2_directory_entry_t) + strlen(e->name);
			new_size = align_to(new_size, 4);

			e->entry_size = new_size;
			memcpy(block + offset, e, new_size);
			offset += new_size;
			kfree(e);

			break;
		}

		offset += e->entry_size;
		kfree(e);
	} while (offset < block_size);

	/* Create a new null inode */
	uint32_t new_inode = ext2_allocate_inode();

	inode_t new_in;
	memset(&new_in, 0, sizeof(inode_t));

	// We fill the default entries
	if (type == DTYPE_DIR) {
		uint8_t* tmp = kmalloc(block_size);
		ext2_directory_entry_t* dots = kmalloc(sizeof(ext2_directory_entry_t) + 2);

		strncpy(dots->name, ".", 1);
		*dots = (ext2_directory_entry_t) {
			.inode = new_inode,
			.type = DTYPE_DIR,
			.name_len_low = 1,
			.entry_size = 12
		};

		memcpy(tmp, dots, dots->entry_size);
		strncpy(dots->name, "..", 2);
		dots->inode = parent_inode;
		dots->name_len_low = 2;
		dots->entry_size = block_size - 12;

		memcpy(tmp + 12, dots, dots->entry_size);
		new_in.dbp[0] = ext2_allocate_block();
		ext2_write_inode_block(&new_in, 0, tmp);
		kfree(tmp);

		new_in.size_lower = block_size;
	}

	ext2_update_inode(new_inode, &new_in);

	/* Add the entry referencing that inode */
	uint32_t size = sizeof(ext2_directory_entry_t) + strlen(name);
	ext2_directory_entry_t* new_entry = kmalloc(size);

	strcpy(new_entry->name, name);
	*new_entry = (ext2_directory_entry_t) {
		.inode = new_inode,
		.type = type,
		.name_len_low = strlen(name),
		.entry_size = 1024 - offset
	};

	memcpy(block + offset, new_entry, size);
	ext2_write_inode_block(in, 0, block);
	kfree(new_entry);
	kfree(block);
	kfree(in);

	return new_inode;
}

/* Create a file named `name` in the directory pointed to by `parent_inode`.
 */
uint32_t ext2_create(const char* name, uint32_t parent_inode) {
	uint32_t inode = ext2_add_directory_entry(name, parent_inode, DTYPE_FILE);
	inode_t* in = ext2_get_inode(inode);

	in->type_perms = INODE_FILE;

	ext2_update_inode(inode, in);
	kfree(in);

	return inode;
}

uint32_t ext2_mkdir(const char* name, uint32_t parent_inode) {
	uint32_t inode = ext2_add_directory_entry(name, parent_inode, DTYPE_DIR);
	inode_t* in = ext2_get_inode(inode);

	in->type_perms = INODE_DIR;

	ext2_update_inode(inode, in);
	kfree(in);

	return inode;
}

/* Appends `size` bytes from `data` to the file pointed to by `inode`.
 * Returns the number of bytes written.
 */
uint32_t ext2_append(uint32_t inode, uint8_t* data, uint32_t size) {
	inode_t* in = ext2_get_inode(inode);
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
	inode_t* in = ext2_get_inode(inode);

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
	superblock = parse_superblock();

	if (!superblock) {
		printf("[EXT2] aborting\n");
		return;
	}

	group_descriptors = parse_group_descriptors();

	printf("[EXT2] Initialized volume of size %d KiB\n", len >> 10);
}