#include <stdio.h>
#include <stdbool.h>

#define BUF_SIZE 512

bool cat(const char* path) {
    char buf[BUF_SIZE] = "";
    FILE* f = fopen(path, "r");

    if (!f) {
        return false;
    }

    while (true) {
        int read = fread(buf, 1, BUF_SIZE - 1, f);
        buf[read] = '\0';

        printf("%s", buf);

        if (read < BUF_SIZE - 1) {
            break;
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("usage: %s FILE...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (!cat(argv[i])) {
            printf("%s: failed to open '%s'\n", argv[0], argv[i]);
            return 2;
        }
    }

    return 0;
}