#include <ctype.h>

int isxdigit(int c) {
	return isdigit(c)
		|| (c >= 'a' && c <= 'f')
		|| (c >= 'A' && c <= 'F');
}
