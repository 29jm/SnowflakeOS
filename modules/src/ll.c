#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define STATR 4
#define STATW 2
#define STATX 1

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
    struct stat statent;
    char sec[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    if (!d) {
        printf("%s: failed to open '%s'\n", argv[0], dir);
        return 1;
    }

    printf("\n");

    while ((dent = readdir(d))) {
        stat(dent->d_name, &statent);
        sec[0] = dent->d_type == 2 ? 'd' : '-';
        sec[1] = statent.st_mode & (STATR << 6) ? 'r' : '-';
        sec[4] = statent.st_mode & (STATR << 3) ? 'r' : '-';
        sec[7] = statent.st_mode & STATR ? 'r' : '-';
        sec[2] = statent.st_mode & (STATW << 6) ? 'w' : '-';
        sec[5] = statent.st_mode & (STATW << 3) ? 'w' : '-';
        sec[8] = statent.st_mode & STATW ? 'w' : '-';
        sec[3] = statent.st_mode & (STATX << 6) ? 'x' : '-';
        sec[6] = statent.st_mode & (STATX << 3) ? 'x' : '-';
        sec[9] = statent.st_mode & STATX ? 'x' : '-';
        sec[10] = 0;

        printf("%-11s %-16s %10u\n", sec, dent->d_name, statent.st_size);
        free(dent);
    }

    return 0;
}
