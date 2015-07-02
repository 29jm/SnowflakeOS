#include <ctype.h>

int ispunct(int c) {
	return (c >= 33 && c <= 47)
		|| (c >= 58 && c <= 64)
		|| (c >= 91 && c <= 96)
		|| (c >= 123 && c <= 126);
}
