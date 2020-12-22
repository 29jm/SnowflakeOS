#include <ringbuffer.h>
#include <stdlib.h>

int ringbuffer_init(ringbuffer_t* ref, size_t sz) {
    ref->sz = sz;
    ref->unr_data = 0;

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

ringbuffer_t* ringbuffer_new(size_t sz) {
    ringbuffer_t* ref = malloc(sizeof(ringbuffer_t));
    if (ringbuffer_init(ref, sz) == -1) {
#ifndef _KERNEL_
        free(ref);
#else
        kfree(ref);
#endif
        ref = NULL;
    }

    return ref;
}

size_t ringbuffer_used(ringbuffer_t* ref) {
    return ref->unr_data;
}

size_t ringbuffer_free(ringbuffer_t* ref) {
    return ref->sz - ref->unr_data;
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

int ringbuffer_write(ringbuffer_t* ref, size_t n, uint8_t* data) {
    bool wraps = (ref->w_pos + n) > ref->sz;
    uint8_t* pos = ref->data + ref->w_pos;

    if (ref->w_pos + n == ref->r_pos) {
        return -1;
    }

    for (size_t i = 0; i < n; i++) {
        uint8_t* i_pos = ref->data + i;
        *i_pos = *(data + i);
    }
    ref->w_pos = ((ref->w_pos + n) % ref->sz) == 0 ? n : (ref->w_pos + n);
    ref->unr_data = ref->unr_data + n;
    // returns 0 if the data did fit, 1 if old data was overwritten or
    // -1 if there is insufficient space before the read pointer to place the data
    return !wraps ? 0 : 1;
}

size_t ringbuffer_read(ringbuffer_t* ref, size_t n, uint8_t* buffer) {
    uint8_t byte;
    uint8_t* cp_ptr;
    size_t r_car = ref->r_pos % ref->sz;
    // read n or unr_data if it is less than n
    size_t max = ref->unr_data >= n ? n : ref->unr_data;

    if (ref->unr_data <= 0) {
        return 0;
    }

    for (size_t i = 0; i < max; i++) {
        cp_ptr = (uint8_t*) buffer + i;
        byte = *(ref->data + (r_car + i));
        *cp_ptr = byte;
    }

    ref->unr_data = ref->unr_data - max;
    ref->r_pos = ((ref->r_pos + max) % ref->sz) == 0 ? n : (ref->r_pos + max);
    return max;
}
