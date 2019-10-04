#include <stdio.h>

#ifdef _KERNEL_
#include <kernel/term.h>
#endif

int putchar(int c) {
#ifdef _KERNEL_
	term_putchar(c);
#else
	asm (
		"mov %[c], %%eax\n"
		"int $0x30\n"
		:
		: [c] "r" (c)
		: "%eax"
	);
#endif
	return c;
}
