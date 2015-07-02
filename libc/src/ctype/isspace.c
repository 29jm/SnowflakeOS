#include <ctype.h>

int isspace(int c) {
	return (c == ' ')
		|| (c >= 9 && c <= 13);
}
