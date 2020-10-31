#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>

long int strtol(const char* nptr, char** endptr, int base) {
    int sign = 1;
    char* s = (char*) nptr;

    while (isspace(*s)) {
        s++;
    }

    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    if ((base == 0 || base == 16) && (!strncmp(s, "0x", 2) || !strncmp(s, "0X", 2))) {
        base = 16;
        s += 2;
    } else if (base == 0 && *s == '0') {
        base = 8;
        s++;
    } else if (base == 0) {
        base = 10;
    }

    char* end = s;
    char max_val = 0;

    if (base <= 10) {
        max_val = '0' + base;
    } else {
        max_val = 'a' + (base - 11);
    }

    while (*end) {
        if (!isalnum(*end) || tolower(*end) > max_val) {
            break;
        }

        end++;
    }

    if (endptr) {
        if (end == s) {
            *endptr = (char*) nptr;

            return 0;
        }

        *endptr = end;
    }

    uint32_t len = end - s;
    uint32_t val = 0;
    char* current_digit = s;

    while (current_digit != end) {
        uint32_t coeff = 0;

        if (isalpha(*current_digit)) {
            coeff = tolower(*current_digit) - 'a' + 10;
        } else {
            coeff = *current_digit - '0';
        }

        val += coeff * powi(base, len - 1 - (current_digit - s));
		current_digit++;
    }

    return sign * val;
}
