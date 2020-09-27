#include <kernel/fs.h>
#include <kernel/ext2.h>
#include <kernel/list.h>
#include <kernel/sys.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static list_t* file_table;
static int32_t current_fd;

void init_fs() {
	file_table = list_new();
}

int32_t fs_open(const char* path, uint32_t mode) {
	if (mode != FS_READ) {
		printf("[FS] write mode unsupported\n");
		return -1;
	}

	uint32_t inode = ext2_open(path);

	if (!inode) {
		printf("fs_open %s: -1\n", path);
		return -1;
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

void fs_print_open() {
	printf("[fs] open file descriptors: ");

	for (uint32_t i = 0; i < file_table->count; i++) {
		printf("%d -> ", ((ft_entry_t*) list_get_at(file_table, i))->fd);
	}

	printf("end\n");
}

void fs_close(int32_t fd) {
	for (uint32_t i = 0; i < file_table->count; i++) {
		ft_entry_t* entry = list_get_at(file_table, i);

		if (entry->fd == fd) {
			list_remove_at(file_table, i);
			kfree(entry);
			return;
		}
	}
}

uint32_t fs_read(int32_t fd, uint8_t* buf, uint32_t size) {
	for (uint32_t i = 0; i < file_table->count; i++) {
		ft_entry_t* entry = list_get_at(file_table, i);

		if (entry->fd == fd && entry->mode & FS_READ) {
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

int32_t fs_readdir(int32_t fd, sos_directory_entry_t* d_ent, uint32_t size) {
	for (uint32_t i = 0; i < file_table->count; i++) {
		ft_entry_t* entry = list_get_at(file_table, i);

		if (entry->fd == fd && entry->mode & FS_READ) {
			// printf("[fs] readdir: fd %d, offset %d\n", fd, entry->offset);
			ext2_directory_entry_t* ent = ext2_readdir(entry->inode, entry->offset);

			if (ent == NULL) {
				return -3;
			}

			uint32_t unpadded_size = min(ent->entry_size, 263);

			if (unpadded_size <= size) {
				// TODO: proper assignation depending on fs type
				memcpy(d_ent, ent, unpadded_size);
			} else {
				kfree(ent);

				return -2;
			}

			entry->offset += d_ent->entry_size;
			kfree(ent);

			return 0;
		}
	}

	return -1; // TODO: error codes
}