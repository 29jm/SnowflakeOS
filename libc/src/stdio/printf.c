#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void print(const char* data, size_t data_length)
{
	for (size_t i = 0; i < data_length; i++)
		putchar((int) ((const unsigned char*) data)[i]);
}

int printf(const char* restrict format, ...)
{
	va_list parameters;
	va_start(parameters, format);

	int written = 0;
	size_t amount;
	bool rejected_bad_specifier = false;
	char conversion_buf[128] = "";

	while (*format != '\0') {
		if (*format != '%') {
		print_c:
			amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			print(format, amount);
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format;

		if (*(++format) == '%')
			goto print_c;

		if (rejected_bad_specifier) {
		incomprehensible_conversion:
			rejected_bad_specifier = true;
			format = format_begun_at;
			goto print_c;
		}

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			print(&c, sizeof(c));
		}
		else if (*format == 's') {
			format++;
			const char* s = va_arg(parameters, const char*);
			print(s, strlen(s));
		}
		else if (*format == 'd') {
			format++;
			int i = va_arg(parameters, int);
			const char* s = itoa(i, conversion_buf, 10);
			print(s, strlen(s));
		}
		else if (*format == 'x') {
			format++;
			int i = va_arg(parameters, int);
			itoa(i, conversion_buf, 16);
			print(conversion_buf, strlen(conversion_buf));
		}
		else if (*format == 'X') {
			format++;
			int i = va_arg(parameters, int);
			itoa(i, conversion_buf, 16);
			for (size_t i = 0; i < strlen(conversion_buf); i++)
				conversion_buf[i] = toupper(conversion_buf[i]);
			print(conversion_buf, strlen(conversion_buf));
		}
		else if (*format == 'b') {
			// non standard
			format++;
			int i = va_arg(parameters, int);
			itoa(i, conversion_buf, 2);
			print(conversion_buf, strlen(conversion_buf));
		}
		else {
			goto incomprehensible_conversion;
		}
	}

	va_end(parameters);

	return written;
}
