#include <ringbuffer.h>
#include <stdlib.h>

/* Initialize a ringbuffer for use takes a pointer to a declared ring buffer
 * and initializes it with size size it will be allocated in size * isz
 * returns: 0 on success -1 on failure, if this fails consider the ringbuffer
 * pointed at by ref invalid and unsafe to use errno is likely to have a cause
 * for the failure
 */
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

/* Write n bytes to the ringbuffer pointed at by ref returns 0 if the data did
 * fit, 1 if old data was overwritten or -1 if there is insufficient space before
 * the read pointer to place the data
 */
int ringbuffer_write(ringbuffer_t* ref, size_t n, uint8_t* data) {

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

/* Read n bytes from the ringbuffer pointed at by ref into block returns n if a
 * full block is read vallues < n indicate that there is less data than n
 * available and the returned value was read to buffer a return of 0 means no
 * data was available to read also note that buffer is not cleared between reads
 */
size_t ringbuffer_read(ringbuffer_t* ref, size_t n, uint8_t* buffer) {
    size_t r_car = ref->r_pos % ref->size;
    size_t to_read = ref->unread_data >= n ? n : ref->unread_data;

    for (size_t i = 0; i < to_read; i++) {
        buffer[i % ref->size] = ref->data[r_car + i];
    }

    ref->unread_data = ref->unread_data - to_read;
    ref->r_pos = ((ref->r_pos + to_read) % ref->size) == 0 ? n : (ref->r_pos + to_read);
    return to_read;
}
