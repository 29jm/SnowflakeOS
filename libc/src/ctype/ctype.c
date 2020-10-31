#include <ctype.h>

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int isalpha(int c) {
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z');
}

int isblank(int c)  {
    return c == ' ' || c == '\t';
}

int iscntrl(int c) {
    return c >= 0 && c <= 31;
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isgraph(int c) {
    return c >= 33 && c <= 126;
}

int islower(int c) {
    return c >= 'a' && c <= 'z';
}

int isprint(int c) {
    return c >= 32 && c <= 126;
}

int ispunct(int c) {
    return (c >= 33 && c <= 47)
        || (c >= 58 && c <= 64)
        || (c >= 91 && c <= 96)
        || (c >= 123 && c <= 126);
}

int isspace(int c) {
    return (c == ' ')
        || (c >= 9 && c <= 13);
}

int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

int isxdigit(int c) {
    return isdigit(c)
        || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

int tolower(int c) {
    if (!isalpha(c) || islower(c)) {
        return c;
    }

    return c + 32;
}

int toupper(int c) {
    if (!isalpha(c) || isupper(c)) {
        return c;
    }

    return c - 32;
}