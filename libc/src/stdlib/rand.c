#include <stdlib.h>

/* Implementation taken from
 * https://stackoverflow.com/a/4768194
 */

static uint32_t n;

void srand(unsigned int seed) {
    n = seed;
}

int rand() {
    return (((n = n * 214013L + 2531011L) >> 16) & 0x7fff);
}