#include <unistd.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("unlink: missing argument\n");
        return 0;
    }

    printf("unlink %s\n", argv[1]);

    return unlink(argv[1]);
}