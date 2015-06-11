#include <ctype.h>

// Copied from ToaruOS
int atoi(const char* s) {
	int n = 0;
	int neg = 0;

	while (isspace(*s)) {
		s++;
	}

	switch (*s) {
		case '-':
			neg = 1;
			/* Fallthrough is intentional here */
		case '+':
			s++;
	}

	while (isdigit(*s)) {
		n = 10*n - (*s++ - '0');
	}
	/* The sign order may look incorrect here but this is correct as n is calculated
	 * as a negative number to avoid overflow on INT_MAX.
	 */
	return neg ? n : -n;
}
