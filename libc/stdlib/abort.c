#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>

__attribute__((__noreturn__))
void abort()
{
	// TODO: Add proper kernel panic.
	printf("Kernel Panic: abort()\n");
	while (1) {}
	__builtin_unreachable();
}
