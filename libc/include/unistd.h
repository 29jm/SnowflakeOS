#pragma once

#include <stddef.h>

#ifndef _KERNEL_
int chdir(const char* path);
char* getcwd(char* buf, size_t size);
int unlink(const char* path);
#endif