#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define RINGBUFFER_OK 0
#define RINGBUFFER_OVERFLOW 1


/**
 * ringbuffer type for SnowflakeOS, this structure deals in bytes
 * as such it is untyped and special care must be taken to
 * read/write consistent lengths or do some separate bookkeeping
 */
typedef struct _ringbuffer {
    size_t size;
    size_t unread_data;
    size_t w_pos;
    size_t r_pos;
    uint8_t* data;
} ringbuffer_t;

/**
 * initialize a ringbuffer for use
 * takes a pointer to a declared ring buffer and initializes it with size size
 * it will be allocated in size * isz
 * returns: 0 on success -1 on failure, if this fails consider
 * the ringbuffer pointed at by ref invalid and unsafe to use
 * errno is likely to have a cause for the failure
 */
int ringbuffer_init(ringbuffer_t* ref, size_t size);

/**
 * allocate and initialize a ringbuffer
 */
ringbuffer_t* ringbuffer_new(size_t size);

/**
 * get the size of unread data in the buffer
 */
size_t ringbuffer_used(ringbuffer_t* ref);

/**
 * how much space is available before the
 * writing more data would overwrite data
 * that has not been read yet
 */
size_t ringbuffer_available(ringbuffer_t* ref);

/**
 * disposes of the ringbuff freeing it's allocated memory
 */
void ringbuffer_free(ringbuffer_t* ref);

/**
 * write n bytes to the ringbuffer pointed at by ref
 * returns 0 if the data did fit, 1 if old data was overwritten or
 * -1 if there is insufficient space before the read pointer to place the data
 */
int ringbuffer_write(ringbuffer_t* ref, size_t n, uint8_t* data);

/**
 * read n bytes from the ringbuffer pointed at by ref into block 
 * returns n if a full block is read vallues < n indicate
 * that there is less data than n available and
 * the returned value was read to buffer
 * a return of 0 means no data was available to read
 * also note that buffer is not cleared of data after n or the returned value
 */
size_t ringbuffer_read(ringbuffer_t* ref, size_t n, uint8_t* buffer);
