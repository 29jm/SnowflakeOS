#pragma once

#include <kernel/uapi/uapi_fs.h>

#include <stddef.h>

#define STDOUT_FILENO FS_STDOUT_FILENO

#ifndef _KERNEL_

int chdir(const char* path);
char* getcwd(char* buf, size_t size);
int unlink(const char* path);

#endif