#include <ctype.h>

int iscntrl(int c) {
	return c >= 0 && c <= 31;
}
