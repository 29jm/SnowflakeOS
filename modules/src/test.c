#include <snow.h>

#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>

#include <sys/stat.h>

#define BUF_SIZE 4096

void tree(char* path, int level) {
    static char p[256] = "\0";
    DIR* d = NULL;
    struct dirent* e = NULL;

    if (p[0] == 0) {
        strcpy(p, path);
    }

    d = opendir(p);

    while (d && (e = readdir(d))) {
        for (int i = 0; i < level; i++) {
            printf("  ");
        }

        printf("- %s%s (%d)\n", e->d_name, e->d_type == 2 ? "/" : "", e->d_ino);

        if (e->d_type == 2 && e->d_name[0] != '.') {
            int nl = strlen(e->d_name);

            if (strcmp(p, "/") != 0) {
                strcat(p, "/");
            }

            strcat(p, e->d_name);
            tree(p, level + 1);
            p[strlen(p) - nl - 1] = 0;
        }

        free(e);
    }

    closedir(d);
}

int main() {
    char buf[BUF_SIZE] = "none";

    mkdir("/biloute", 0);
    mkdir("/biloute/blyat", 0);
    FILE* f2 = fopen("/biloute/baloute", "w");
    fclose(f2);

    FILE* w = fopen("/created", "w");
    char str[] = "Hello writing world\n";
    fwrite(str, strlen(str), 1, w);
    fclose(w);

    FILE* f = fopen("/created", "r");
    int c, i = 0;

    while ((c = fgetc(f)) != EOF && i < BUF_SIZE-1) {
        buf[i++] = c;
    }

    buf[i] = '\0';

    printf("cat /created\n%s", buf);
    fclose(f);

    printf("Treeing /:\n");
    tree("/", 0);

    return 0;
}