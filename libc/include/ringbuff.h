#pragma once
#include <stdbool.h>
#include <stdint.h>

#define RINGBUFF_END -1

struct ringbuff {
    uint32_t sz;
    uint32_t used;
    uint8_t* data;
    uint8_t* w_ptr;
    uint8_t* r_ptr;
};

/**
 * this macro declares a stack allocated ring buffer
 * needs a call to stack_ringbuff_init before use
 * usage example:
 * stack_ringbuff(2048)
 * stack_ringbuff_2048 is now a struct that you can use
 */
#define stack_ringbuff(SZ)                                                                         \
    struct stack_ringbuff_SZ {                                                                     \
        uint32_t sz;                                                                               \
        uint32_t used;                                                                             \
        uint8_t data[SZ];                                                                          \
        uint8_t* w_ptr;                                                                            \
        uint8_t* r_ptr;                                                                            \
    };

/**
 * initialize a ringbuffer for use
 * takes a pointer to a declared ring buffer and initializes it with size sz
 * returns: 0 on success -1 on failure, if this fails consider
 * the ringbuff pointed at by ref invalid and unsafe to use
 * errno is likely to have a cause for the failure
 */
int ringbuff_init(struct ringbuff* ref, uint32_t sz);

/**
 * disposes of the ringbuff freeing it's memory
 */
int ringbuff_dispose(struct ringbuff* ref);
