#include <kernel/uapi/uapi_fs.h>

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <snow.h>

DIR* opendir(const char* path) {
	DIR* dir = malloc(sizeof(DIR));

	if (!dir) {
		printf("[dirent] panic\n");
		return NULL;
	}

	printf("c");
	dir->stream = fopen(path, "r");
	printf("d");

	if (!dir->stream) {
		free(dir);

		return NULL;
	}

	strcpy(dir->name, path);
	dir->fd = dir->stream->fd;

	return dir;
}

struct dirent* readdir(DIR* dir) {
	uint32_t ent_size = sizeof(sos_directory_entry_t) + MAX_PATH;
	sos_directory_entry_t* dir_entry = malloc(ent_size);
	dir_entry->entry_size = ent_size;

	int32_t err = syscall2(SYS_READDIR, dir->fd, (uintptr_t) dir_entry);

	if (err) {
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

int closedir(DIR* dir) {
	if (!dir) {
		return -1;
	}

	fclose(dir->stream);
	free(dir->name);
	free(dir);

	return 0;
}