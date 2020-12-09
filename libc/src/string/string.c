#include <string.h>
#include <stdint.h>
#include <stdlib.h>

uint32_t strlen(const char* string) {
    uint32_t result = 0;

    while (string[result])
        result++;

    return result;
}

char* strcpy(char* dest, const char* src) {
    uint32_t i;

    for (i = 0; i < strlen(src); i++) {
        dest[i] = src[i];
    }

    dest[i] = '\0'; // Null terminator

    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    uint32_t len = strlen(src);
    uint32_t max = len > n ? n : len;

    for (uint32_t i = 0; i < max; i++) {
        dest[i] = src[i];
    }

    return dest;
}

char* strcat(char* dest, const char* src) {
    return strcpy(&dest[strlen(dest)], src);
}

char* strdup(const char* s) {
#ifndef _KERNEL_
    char* buff = (char*) malloc((strlen(s)+1)*sizeof(char));
#else
    char* buff = (char*) kmalloc((strlen(s)+1)*sizeof(char));
#endif

    return strcpy(buff, s);
}

char* strndup(const char* s, size_t n) {
    uint32_t size = 0;

    while (size < n && s[size]) {
        size++;
    }

    char* buff = (char*) malloc(size + 1);
    buff[size] = '\0';

    return (char*) strncpy(buff, s, size);
}

char* strchr(const char* s, int c) {
    int n = strlen(s);

    for (int i = 0; i < n; i++) {
        if (s[i] == c) {
            return (char*) &s[i];
        }
    }

    return NULL;
}

char* strchrnul(const char* s, int c) {
    int n = strlen(s);

    for (int i = 0; i < n; i++) {
        if (s[i] == c) {
            return (char*) &s[i];
        }
    }

    return (char*) &s[n]; // discard const qualifier
}

char* strrchr(const char* s, int c) {
    uint32_t n = strlen(s);

    if (!n) {
        return NULL;
    }

    for (int i = n - 1; i >= 0; i--) {
        if (s[i] == c) {
            return (char*) &s[i];
        }
    }

    return NULL;
}

char* strstr(const char* haystack, const char* needle) {
    uint32_t n = strlen(haystack);
    uint32_t m = strlen(needle);

    if (m > n) {
        return NULL;
    } else if (m == n && !strncmp(haystack, needle, n)) {
        return (char*) &haystack[0];
    }

    for (uint32_t i = 0; i < n - m; i++) {
        for (uint32_t j = 0; haystack[i + j] == needle[j]; j++) {
            if (j == m - 1) {
                return (char*) &haystack[i];
            }
        }
    }

    return NULL;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }

    if (n == 0) {
        return 0;
    }

    return *s1 - *s2;
}

/* For <strings.h>
 */
int strncasecmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && tolower(*s1) == tolower(*s2)) {
        s1++;
        s2++;
        n--;
    }

    if (n == 0) {
        return 0;
    }

    return *s1 - *s2;
}

int strcasecmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}
