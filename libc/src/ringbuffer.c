#include <ringbuffer.h>
#include <stdlib.h>
#include <math.h>

/* Initilize a ringbuffer that was pre-allocated */
int ringbuffer_init(ringbuffer_t* ref, size_t size) {
    ref->size = size;
    ref->r_base = 0;
    ref->data_size = 0;
    ref->data = malloc(size);

    if (ref->data == NULL) {
        return -1;
    }

    return 0;
}

/* Allocate and initialize a ringbuffer returns NULL on failure to allocate */
ringbuffer_t* ringbuffer_new(size_t size) {
    ringbuffer_t* ref = malloc(sizeof(ringbuffer_t));

    if (ref == NULL) {
        return NULL;
    }

    if (ringbuffer_init(ref, size) == -1) {
        free(ref);
        ref = NULL;
    }

    return ref;
}

/* How much data is available in the buffer
 */
size_t ringbuffer_available(ringbuffer_t* ref) {
    return ref->data_size;
}

void ringbuffer_free(ringbuffer_t* ref) {
    free(ref->data);
    free(ref);
}

/* Attempt to write n bytes of data into the buffer ref, returns the number of
 * bytes written
 */
size_t ringbuffer_write(ringbuffer_t* ref, size_t n, uint8_t* buffer) {
    size_t w_base = ref->r_base + ref->data_size;

    for (size_t i = 0; i < n; i++) {
        ref->data[(w_base + i) % ref->size] = buffer[i];
    }

    /* Have we erased old data? */
    if (n > ref->size - ringbuffer_available(ref)) {
        ref->r_base = (w_base + n) % ref->size;
    }

    ref->data_size = min(ref->data_size + n, ref->size);

    return min(n, ref->size);
}

/* Read n bytes from the ringbuffer pointed at by ref into block returns n if a
 * full block is read values < n indicate that there is less data than n
 * available and the returned value was read to buffer a return of 0 means no
 * data was available to read also note that buffer is not cleared so any data
 * after the read bytes is untouched.
 */
size_t ringbuffer_read(ringbuffer_t* ref, size_t n, uint8_t* buffer) {
    size_t to_read = min(n, ringbuffer_available(ref));

    for (size_t i = 0; i < to_read; i++) {
        buffer[i] = ref->data[(ref->r_base + i) % ref->size];
    }

    ref->r_base = (ref->r_base + to_read) % ref->size;
    ref->data_size -= to_read;

    return to_read;
}
