#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

#include <snow.h>

int main(int argc, char* argv[]) {
    char dir[MAX_PATH] = "";

    if (argc > 1) {
        if (!strcmp(argv[1], "--help")) {
            printf("usage: %s [ dir ]\n", argv[0]);
            return 0;
        }

        strcpy(dir, argv[1]);
    } else {
        getcwd(dir, MAX_PATH);
    }

    DIR* d = opendir(dir);
    struct dirent* dent = NULL;

    if (!d) {
        printf("%s: failed to open '%s'\n", argv[0], dir);
        return 1;
    }

    while ((dent = readdir(d))) {
        printf("%s%s\n", dent->d_name, dent->d_type == 2 ? "/" : "");
        free(dent);
    }

    return 0;
}