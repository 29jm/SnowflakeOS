#include <stdio.h>

#include <snow.h>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("usage: %s SOURCE DEST\n", argv[0]);
        return 1;
    }

    int ret = rename(argv[1], argv[2]);

    if (ret) {
        printf("%s: failed\n");
        return 2;
    }

    return 0;
}