#include <ctype.h>

int toupper(int c) {
	if (!isalpha(c))
		return c;
	if (c >= 'a' && c <= 'z')
		return c - 32;
	return c;
}
