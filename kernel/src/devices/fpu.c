#include <kernel/fpu.h>

#include <string.h>

#define CR0_MP (1 << 1)
#define CR0_EM (1 << 2)
#define CR4_OSFXSR (1 << 9)

/* Instructions to read and write FPU context require a 16-bytes aligned buffer */
static uint8_t kernel_fpu[512] __attribute__((aligned(16)));

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

/* Whatever process got interrupted last has its fpu state in kernel_fpu.
 * When this function is called, it is being switched away from, hence we save
 * kernel_fpu in that process"s structure, and we load the next process's fpu
 * state into kernel_fpu, which will get picked up by fpu_kernel_exit soon
 * enough.
 */
void fpu_switch(process_t* prev, const process_t* next) {
    memcpy(prev->fpu_registers, kernel_fpu, 512);
    memcpy(kernel_fpu, next->fpu_registers, 512);
}

/* Called when execution enters the kernel: the fpu state is saved, then
 * cleared, so the kernel gets a fresh start.
 */
void fpu_kernel_enter() {
    asm volatile(
        "fxsave (%0)\n"
        "fninit\n" :: "r" (kernel_fpu));
}

/* Restores the process's fpu state upon returning from the kernel.
 */
void fpu_kernel_exit() {
    asm volatile("fxrstor (%0)\n" :: "r" (kernel_fpu));
}