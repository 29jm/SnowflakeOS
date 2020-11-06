#include <stdio.h>

/* Prints out its arguments in order, for tests purposes.
 */

int main(int argc, char* argv[]) {
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    return 0;
}