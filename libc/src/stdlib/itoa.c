#include <stdlib.h>
#include <stdbool.h>

char* reverse(char* str, int length) {
	int start = 0;
	int end = length - 1;

	while (start < end) {
		char temp = *(str+start);
		*(str+start) = *(str+end);
		*(str+end) = temp;
		start++;
		end--;
	}

	return str;
}

char* itoa(int num, char* str, int base) {
	int i = 0;

	if (num == 0) {
		str[0] = '0';
		str[1] = '\0';
		return str;
	}

	if (base == 10) {
		bool negative = false;

		if (num < 0) {
			negative = true;
			num = -num;
		}

		while (num != 0) {
			int rem = num % base;
			str[i++] = '0' + rem;
			num /= base;
		}

		if (negative) {
			str[i++] = '-';
		}
	} else {
		uint32_t val = (uint32_t) num;

		while (val != 0) {
			int rem = val % base;
			str[i++] = (rem > 9) ? 'a' + (rem - 10) : '0' + rem;
			val /= base;
		}
	}

	str[i] = '\0';

	return reverse(str, i);
}