#include <mm_malloc.h>
#include <ringbuff.h>

int ringbuff_init(struct ringbuff* ref, uint32_t sz) {
    ref->sz = sz;
    ref->used = 0;

// alloc the needed size for this ringbuff and use kmalloc
// if this is compiled with the proper flag
#ifndef _KERNEL_
    ref->data = (uint8_t*) malloc(sz);
#else
    ref->data = (uint8_t*) kmalloc(sz);
#endif

    if (ref->data == NULL) {
        return -1;
    }

    ref->r_ptr = &ref->data;
    ref->w_ptr = &ref->data;
    
    return 0;
}
