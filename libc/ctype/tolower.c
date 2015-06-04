#include <ctype.h>

int tolower(int c) {
	if (!isalpha(c))
		return c;
	if (c >= 'A' && c <= 'Z')
		return c + 32;
	return c;
}
