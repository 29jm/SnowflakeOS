#ifndef SYS_H
#define SYS_H

#define UNUSED(param) (void) param

#define BREAK() do { \
                	asm ("xchgw %bx, %bx\n"); \
                } while (false)

/* Returns the next multiple of `s` greater than `a`, or `a` if it is a
 * multiple of `s`.
 */
#define ALIGN(a, s) (((a)/(s) + ((a) % (s) ? 1 : 0)) * (s))

#endif
