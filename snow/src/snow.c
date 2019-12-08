#include <snow.h>

void* snow_alloc(uint32_t size) {
    uintptr_t address;

    asm (
        "mov $4, %%eax\n"
        "mov %[size], %%ebx\n"
        "int $0x30\n"
        "mov %%eax, %[address]\n"
        : [address] "=r" (address)
        : [size] "r" (size)
        : "%eax");

    return (void*) address;
}

/* TODO: expose framebuffer size etc...
 */
void snow_render(uintptr_t buffer) {
    asm (
        "mov $5, %%eax\n"
        "mov %[buffer], %%ebx\n"
        "int $0x30\n"
        :: [buffer] "r" (buffer));
}