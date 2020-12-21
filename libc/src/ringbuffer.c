#include <mm_malloc.h>
#include <ringbuffer.h>

int ringbuffer_init(ringbuffer_t* ref, uint32_t sz) {
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

    ref->r_pos = 0;
    ref->w_pos = 0;

    return 0;
}

int ringbuffer_dispose(ringbuffer_t* ref) {
#ifndef __KERNEL__
    free(ref->data);
    free(ref);
#else
    kfree(ref->data);
    kfree(ref);
#endif
}

int ringbuffer_write(ringbuffer_t* ref, void* data, size_t n) {
    bool wraps = (ref->w_pos + n) > ref->sz;
    if (ref->w_pos + n == ref->used - 1) {
        return -1;
    }
    uint8_t* pos = ref->data + ref->w_pos;
    for (size_t i = 0; i < n; i++) {
        uint8_t* i_pos = pos[i];
        *i_pos = data + i;
    }
    ref->w_pos = (ref->w_pos + n) % ref->sz;
    ref->used = ref->r_pos - ref->w_pos;
    // returns 0 if the data did fit, 1 if old data was overwritten or
    // -1 if there is insufficient space before the read pointer to place the data
    return !wraps ? 0 : 1;
}
