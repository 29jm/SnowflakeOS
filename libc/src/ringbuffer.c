#include <ringbuffer.h>
#include <stdlib.h>
#include <math.h>

/* Initilizes a ringbuffer that was pre-allocated. Useful when you'd rather use
 * the stack than allocate.
 * Obviously, don't call `ringbuffer_free` on those.
 */
ringbuffer_t* ringbuffer_init(ringbuffer_t* ref, uint8_t* buf, size_t size) {
    ref->data = buf;
    ref->size = size;
    ref->r_base = 0;
    ref->data_size = 0;

    return ref;
}

/* Allocate and initialize a ringbuffer, returns NULL on failure. */
ringbuffer_t* ringbuffer_new(size_t size) {
    ringbuffer_t* ref = malloc(sizeof(ringbuffer_t));

    if (ref == NULL) {
        return NULL;
    }

    uint8_t* buf = zalloc(size);

    if (!buf) {
        return NULL;
    }

    return ringbuffer_init(ref, buf, size);
}

/* Returns how much data is available in the buffer, in bytes.
 */
size_t ringbuffer_available(ringbuffer_t* ref) {
    return ref->data_size;
}

/* Frees a ringbuffer previously allocated by `ringbuffer_new`.
 */
void ringbuffer_free(ringbuffer_t* ref) {
    free(ref->data);
    free(ref);
}

/* Writes `n` bytes into the ringbuffer.
 * Returns the number of new bytes available. This may not equal `n` because
 * overwriting replaces data, it doesn't add more.
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

/* Read at most n bytes from the ringbuffer into `buffer`.
 * Returns the total number of bytes read. If only k < n bytes are available,
 * k bytes are read and k is returned.
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
