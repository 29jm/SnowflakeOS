#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

double strtod(const char* nptr, char** endptr) {
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

    char* end = s;
    char* dot_pos = NULL;
    bool dot_found = false;
    char* exp_pos = NULL;
    bool exp_found = false;
    bool expsign_found = false;

    while (*end) {
        if (!strchr(".+-eE", *end) && !isdigit(*end)) {
            break;
        }

        if ((*end == '.' && dot_found) || (tolower(*end) == 'e' && exp_found)
            || (strchr("+-", *end) && expsign_found)) {
            break;
        }

        if (*end == '.') {
            dot_pos = end;
            dot_found = true;
        } else if (tolower(*end) == 'e') {
            exp_pos = end;
            exp_found = true;
        } else if (strchr("+-", *end)) {
            expsign_found = true;
        }

        end++;
    }

    if (!dot_pos) {
        dot_pos = exp_pos ? exp_pos : end;
    }

    if (endptr) {
        if (end == s) {
            *endptr = (char*) nptr;

            return 0;
        }

        *endptr = end;
    }

    double val = 0.0;
    char* current_digit = s;

    while (current_digit != end) {
        int power = dot_pos - current_digit - 1;

        if (tolower(*current_digit) == 'e') {
            break;
        }

        if (current_digit == dot_pos) {
            current_digit++;
            continue;
        }

        if (current_digit > dot_pos) {
            power += 1;
        }

        int digit = *current_digit - '0';

        val += digit * pow(10, power);
        current_digit++;
    }

    if (tolower(*current_digit) == 'e') {
        current_digit++;

        char* exp_end;
        int exponent = strtol(current_digit, &exp_end, 10);

        if (exp_end == current_digit) {
            if (endptr) {
                *endptr = (char*) nptr;
            }

            return 0;
        }

        val *= pow(10, exponent);
    }

    return sign * val;
}
