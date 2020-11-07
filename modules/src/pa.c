#include <stdio.h>
#include <list.h>

/* Prints out its arguments in order, for tests purposes.
 */

int main(int argc, char* argv[]) {
    list_t list = LIST_HEAD_INIT(list);

    for (int i = 0; i < 10; i++) {
        list_add(&list, (void*) (i*i));
    }

    for (int i = 0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    return 0;
}