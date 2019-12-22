#include <stdio.h>

#ifdef _KERNEL_
#include <kernel/term.h>
#include <kernel/serial.h>
#endif

int putchar(int c) {
#ifdef _KERNEL_
	term_putchar(c);
	serial_write((char) c);
#else
	asm (
		"mov $3, %%eax\n"
		"mov %[c], %%ecx\n"
		"int $0x30\n"
		:: [c] "r" (c)
		: "%eax"
	);
#endif
	return c;
}
