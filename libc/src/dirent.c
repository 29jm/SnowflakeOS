#ifndef _KERNEL_

#include <kernel/uapi/uapi_fs.h>

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <snow.h>

/* Opens the directory pointed to by `path` and returns a directory handle.
 * This handle can later be freed by calling `closedir`.
 * Returns NULL on error.
 */
DIR* opendir(const char* path) {
	DIR* dir = malloc(sizeof(DIR));

	if (!dir) {
		printf("[dirent] panic\n");
		return NULL;
	}

	dir->stream = fopen(path, "r");

	if (!dir->stream) {
		free(dir);

		return NULL;
	}

	strcpy(dir->name, path);
	dir->fd = dir->stream->fd;

	return dir;
}

/* Returns the next entry in the given directory stream.
 * Returns NULL when no more entries are present.
 */
struct dirent* readdir(DIR* dir) {
	uint32_t ent_size = sizeof(sos_directory_entry_t) + MAX_PATH;
	sos_directory_entry_t* dir_entry = malloc(ent_size);
	dir_entry->entry_size = ent_size;

	uint32_t read = syscall2(SYS_READDIR, dir->fd, (uintptr_t) dir_entry);

	if (!read) {
		free(dir_entry);

		return NULL;
	}

	struct dirent* d_ent = calloc(sizeof(struct dirent));
	d_ent->d_ino = dir_entry->inode;
	strcpy(d_ent->d_name, dir_entry->name);
	d_ent->d_type = dir_entry->type;

	free(dir_entry);

	return d_ent;
}

/* Closes a directory stream previously returned by `opendir`.
 */
int closedir(DIR* dir) {
	if (!dir) {
		return -1;
	}

	fclose(dir->stream);
	free(dir->name);
	free(dir);

	return 0;
}

#endif