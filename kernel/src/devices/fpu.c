#include <kernel/fpu.h>

#include <string.h>

/* Instructions to read and write FPU context require a 16-bytes aligned buffer.
 */
static uint8_t aligned_buff[512] __attribute__ ((aligned (16)));

void init_fpu() {
    asm volatile ("fninit");
}

/* Saves FPU context for `prev` and restores `next`'s.
 */
void fpu_switch(process_t* prev, const process_t* next) {
    asm volatile ("fxrstor (%0)" :: "r" (aligned_buff));
    memcpy(prev->fpu_registers, aligned_buff, 512);

    memcpy(aligned_buff, next->fpu_registers, 512);
    asm volatile ("fxsave (%0)" :: "r" (aligned_buff));
}