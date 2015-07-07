#include <stdio.h>

#ifdef _KERNEL_
#include <kernel/tty.h>
#endif

int putchar(int ic)
{
#ifdef _KERNEL_
	char c = (char) ic;
	term_write(&c, sizeof(c));
#else
	// TODO: You need to implement a write system call.
#endif
	return ic;
}
