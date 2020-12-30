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
    ref->unread_data = 0;
    ref->r_pos = 0;
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

size_t ringbuffer_used(ringbuffer_t* ref) {
    return ref->unread_data;
}

/* How much space is available before the writing more data would overwrite data
 * that has not been read yet
 */
size_t ringbuffer_available(ringbuffer_t* ref) {
    return ref->size - ref->unread_data;
}

void ringbuffer_free(ringbuffer_t* ref) {
    free(ref->data);
    free(ref);
}

/* Attempt to write n bytes of data into the buffer ref, returns the number of
 * bytes written
 */
size_t ringbuffer_write(ringbuffer_t* ref, size_t n, uint8_t* buffer) {
    size_t w_pos = (ref->r_pos + n) % ref->size;
    printf("w_pos %d, r_pos %d, unread_data %d\n", w_pos, ref->r_pos, ref->unread_data);

    for (size_t i = 0; i < n; i++) {
        ref->data[(w_pos + i) % ref->size] = buffer[i];
    }

    // /* Have we erased old data? */
    // if (n > ringbuffer_available(ref)) {
    // ref->r_pos = (w_pos + n) % ref->size;
    // }

    ref->unread_data = min(ref->unread_data + n, ref->size);

    return min(n, ref->size);
}

/* Read n bytes from the ringbuffer pointed at by ref into block returns n if a
 * full block is read values < n indicate that there is less data than n
 * available and the returned value was read to buffer a return of 0 means no
 * data was available to read also note that buffer is not cleared so any data
 * after the read bytes is untouched.
 */
size_t ringbuffer_read(ringbuffer_t* ref, size_t n, uint8_t* buffer) {
    size_t to_read = min(n, ref->unread_data);

    printf("r_pos %d, unread_data %d\n", ref->r_pos, ref->unread_data);

    for (size_t i = 0; i < to_read; i++) {
        buffer[i] = ref->data[(ref->r_pos + i) % ref->size];
    }

    ref->unread_data = ref->unread_data - to_read;
    ref->r_pos = (ref->r_pos + to_read) % ref->size;

    return to_read;
}
