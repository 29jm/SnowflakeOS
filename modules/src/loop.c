#include <stdbool.h>

int main() {
    while (true) {
        asm volatile (
            "mov $0, %eax\n"
            "mov $1, %ebx\n"
            "mov $2, %ecx\n"
            "mov $3, %edx\n"
        );
    }
}