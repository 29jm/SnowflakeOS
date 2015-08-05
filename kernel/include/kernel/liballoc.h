#ifndef _LIBALLOC_H
#define _LIBALLOC_H

#include <stdint.h>

#define PREFIX(func) k ## func

int liballoc_lock();
int liballoc_unlock();
void* liballoc_alloc(uint32_t num);
int liballoc_free(void* virt, uint32_t num);

void* PREFIX(malloc)(size_t);
void* PREFIX(realloc)(void*, size_t);
void* PREFIX(calloc)(size_t, size_t);
void PREFIX(free)(void*);

#endif
