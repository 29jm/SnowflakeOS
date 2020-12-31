#include <ringbuffer.h>
#include <stdlib.h>
#include <math.h>

/* Initialize a ringbuffer for use takes a pointer to a declared ring buffer
 * and initializes it with size size it will be allocated in size * isz
 * returns: 0 on success -1 on failure, if this fails consider the ringbuffer
 * pointed at by ref invalid and unsafe to use errno is likely to have a cause
 * for the failure
 */
int ringbuffer_init(ringbuffer_t* ref, size_t size) {
    ref->size = size;
    ref->w_base = 0;
    ref->r_base = 0;
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
    long distance;

    distance = ref->w_base - ref->r_base;
    return distance >= 0 ? distance : distance * -1;
}

void ringbuffer_free(ringbuffer_t* ref) {
    free(ref->data);
    free(ref);
}

/* Attempt to write n bytes of data into the buffer ref, returns the number of
 * bytes written
 */
size_t ringbuffer_write(ringbuffer_t* ref, size_t n, uint8_t* buffer) {

    for (size_t i = 0; i < n; i++) {
        ref->data[(ref->w_base + i) % ref->size] = buffer[i];
    }

    // /* Have we erased old data? */
    // if (n > ringbuffer_available(ref)) {
    // ref->r_pos = (w_pos + n) % ref->size;
    // }

    ref->w_base = (ref->w_base + n) % ref->size;
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

    return to_read;
}
