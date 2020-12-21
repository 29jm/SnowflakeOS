#pragma once
#include <stdbool.h>
#include <stdint.h>

#define RINGBUFFER_EOD -1
typedef unsigned long int size_t;

/**
 * ringbuffer type for SnowflakeOS, this structure deals in bytes
 * as such it is untyped and special care must be taken to
 * read/write consistent lengths or do some separate bookkeeping
 */
typedef struct _ringbuffer {
    size_t sz;
    size_t used;
    uint8_t* data;
    size_t w_pos;
    size_t r_pos;
} ringbuffer_t;

/**
 * initialize a ringbuffer for use
 * takes a pointer to a declared ring buffer and initializes it with size sz
 * it will be allocated in sz * isz
 * returns: 0 on success -1 on failure, if this fails consider
 * the ringbuffer pointed at by ref invalid and unsafe to use
 * errno is likely to have a cause for the failure
 */
int ringbuffer_init(ringbuffer_t* ref, size_t sz);

/**
 * disposes of the ringbuff freeing it's allocated memory
 */
int ringbuffer_dispose(ringbuffer_t* ref);

/**
 * write n bytes to the ringbuffer pointed at by ref
 * returns 0 if the data did fit, 1 if old data was overwritten or
 * -1 if there is insufficient space before the read pointer to place the data
 */
int ringbuffer_write(ringbuffer_t* ref, void* data, size_t n);

/**
 * read n bytes from the ringbuffer pointed at by ref into block 
 * returns n if a full block is read vallues < n indicate
 * that there is less data than n available and
 * the returned value was read to block
 */
int ringbuffer_read(ringbuffer_t* ref, size_t n, uint8_t* block);
