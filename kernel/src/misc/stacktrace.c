#include <kernel/stacktrace.h>
#include <kernel/sys.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

typedef struct _stackframe_t {
    struct _stackframe_t* ebp;
    uintptr_t eip;
} stackframe_t;

static uint8_t* symbols;
static uint32_t len;

void init_stacktrace(uint8_t* data, uint32_t size) {
    symbols = realloc(data, size + 1); // NULL-terminate the symbol list
    symbols[size] = 0;
    len = size;
}

/* Returns a pointer to the start of the symbol corresponding to `*addr`.
 * Returns the symbol's address in `*addr`.
 */
char* symbol_for_addr(uintptr_t* addr) {
    uintptr_t last = 0, current = 0;
    char* last_sym = 0, * current_sym = 0;
    char* curr = (char*) symbols;

    while (curr < (char*) symbols + len) {
        current = strtol(curr, &current_sym, 16);
        current_sym = current_sym + 1;

        if (!last) {
            last = current;
            last_sym = current_sym;
        }

        if (current > *addr) {
            *addr = last;
            return last_sym;
        }

        last = current;
        last_sym = current_sym;
        curr = strchr(curr, '\n') + 1;
    }

    return NULL;
}

void stacktrace_print() {
    stackframe_t* stackframe = NULL;
    uintptr_t addr = 0;

    asm volatile ("movl %%ebp, %0" : "=r"(stackframe));

    printk("stacktrace:");

    while (stackframe) {
        addr = stackframe->eip;
        char* sym = symbol_for_addr(&addr);
        char* end = strchr(sym, '\n');

        *end = '\0';
        printk(" %p: %s", addr, sym);
        *end = '\n';

        stackframe = stackframe->ebp;
    }
}