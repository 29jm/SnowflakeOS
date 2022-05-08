#include <stdio.h>

#ifdef _KERNEL_
#include <kernel/term.h>
#include <kernel/serial.h>
#else
#include <kernel/uapi/uapi_syscall.h>
#endif

extern int32_t syscall1(uint32_t eax, uint32_t ebx);

/* Nothing to do with libc's putchar; this writes to serial output */
int putchar(int c) {
#ifdef _KERNEL_
    serial_write(c);
    term_putchar(c);
#else
    syscall1(SYS_PUTCHAR, c);
#endif
    return c;
}
