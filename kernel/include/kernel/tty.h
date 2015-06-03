#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>

void terminal_init();
void terminal_reset();
void terminal_putchar_t(char c, size_t x, size_t y);
void terminal_scrolldown();
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);

#endif
