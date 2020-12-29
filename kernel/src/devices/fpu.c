#include <kernel/fpu.h>

#include <string.h>

#define CR0_MP (1 << 1)
#define CR0_EM (1 << 2)
#define CR4_OSFXSR (1 << 9)

/* Instructions to read and write FPU context require a 16-bytes aligned buffer */
static uint8_t aligned_buff[512] __attribute__ ((aligned (16)));

void init_fpu() {
    uint32_t cr;

    /* Configure CR0: disable emulation (EM), as we assume we have an FPU, and
     * enable the EM bit: with the TS and EM bits disabled, `wait/fwait`
     * instructions won't generate exceptions. This last part is desired
     * because we save the FPU state on each task switch. */
    asm volatile(
        "clts\n"
        "mov %%cr0, %0" : "=r"(cr));

    cr &= ~CR0_EM;
    cr |= CR0_MP;

    asm volatile("mov %0, %%cr0" ::"r"(cr));

    /* Configure CR4: enable the `fxsave` and `fxrstor` instructions. */
    asm volatile("mov %%cr4, %0" : "=r"(cr));

    cr |= CR4_OSFXSR;

    asm volatile(
        "mov %0, %%cr4\n"
        "fninit" ::"r"(cr));
}

/* Saves FPU context for `prev` and restores `next`'s.
 */
void fpu_switch(process_t* prev, const process_t* next) {
    asm volatile ("fxsave (%0)" :: "r" (aligned_buff));
    memcpy(prev->fpu_registers, aligned_buff, 512);

    memcpy(aligned_buff, next->fpu_registers, 512);
    asm volatile ("fxrstor (%0)" :: "r" (aligned_buff));
}