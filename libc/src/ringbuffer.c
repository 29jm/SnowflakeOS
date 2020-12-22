#include <ringbuffer.h>
#include <stdlib.h>

int ringbuffer_init(ringbuffer_t* ref, size_t size) {
    ref->size = size;
    ref->unread_data = 0;

    ref->data = (uint8_t*) malloc(size);

    if (ref->data == NULL) {
        return -1;
    }

    ref->r_pos = 0;
    ref->w_pos = 0;

    return 0;
}

ringbuffer_t* ringbuffer_new(size_t size) {
    ringbuffer_t* ref = malloc(sizeof(ringbuffer_t));
    if (ringbuffer_init(ref, size) == -1) {
        free(ref);
        ref = NULL;
    }

    return ref;
}

size_t ringbuffer_used(ringbuffer_t* ref) {
    return ref->unread_data;
}

size_t ringbuffer_available(ringbuffer_t* ref) {
    return ref->size - ref->unread_data;
}

void ringbuffer_free(ringbuffer_t* ref) {
    free(ref->data);
    free(ref);
}

int ringbuffer_write(ringbuffer_t* ref, size_t n, uint8_t* data) {
    uint8_t* pos = ref->data + ref->w_pos;

    if (ref->w_pos + n == ref->r_pos) {
        return -1;
    }

    for (size_t i = 0; i < n; i++) {
        ref->data[i % ref->size] = data[i];
    }
    ref->w_pos = ((ref->w_pos + n) % ref->size) == 0 ? n : (ref->w_pos + n);
    ref->unread_data = ref->unread_data + n;
    // returns 0 if the data did fit, 1 if old data was overwritten
    return !((ref->w_pos + n) > ref->size) ? RINGBUFFER_OK : RINGBUFFER_OVERFLOW;
}

size_t ringbuffer_read(ringbuffer_t* ref, size_t n, uint8_t* buffer) {
    uint8_t byte;
    uint8_t* cp_ptr;
    size_t r_car = ref->r_pos % ref->size;
    // read n or unr_data if it is less than n
    size_t max = ref->unread_data >= n ? n : ref->unread_data;

    if (ref->unread_data <= 0) {
        return 0;
    }

    for (size_t i = 0; i < max; i++) {
        buffer[i % ref->size] = ref->data[(r_car + i)];
    }

    ref->unread_data = ref->unread_data - max;
    ref->r_pos = ((ref->r_pos + max) % ref->size) == 0 ? n : (ref->r_pos + max);
    return max;
}
