#include <stdint.h>

/* Both gcc and clang generate calls to some of these functions when
 * they encounter arithmetic on 64 bits integers, notably in stb_sprintf.
 * Let's not link with -lgcc again.
 * Taken from an unsung hero at https://github.com/glitchub/arith64.
 */
typedef union {
    uint64_t u64;
    int64_t s64;
    struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        uint32_t hi;
        uint32_t lo;
#else
        uint32_t lo;
        uint32_t hi;
#endif
    } u32;
    struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        int32_t hi;
        int32_t lo;
#else
        int32_t lo;
        int32_t hi;
#endif
    } s32;
} _arith64_word;

// extract hi and lo 32-bit words from 64-bit value
#define hi(n) (_arith64_word) {.u64 = n}.u32.hi
#define lo(n) (_arith64_word) {.u64 = n}.u32.lo
#define negate(a, b) (((a) ^ ((((int64_t)(b)) >= 0) - 1)) + (((int64_t)(b)) < 0))
#define abs(a) negate(a, a)

int __clzdi2(uint64_t a) {
    int b, n = 0;
    b = !(a & 0xffffffff00000000ULL) << 5;
    n += b;
    a <<= b;
    b = !(a & 0xffff000000000000ULL) << 4;
    n += b;
    a <<= b;
    b = !(a & 0xff00000000000000ULL) << 3;
    n += b;
    a <<= b;
    b = !(a & 0xf000000000000000ULL) << 2;
    n += b;
    a <<= b;
    b = !(a & 0xc000000000000000ULL) << 1;
    n += b;
    a <<= b;
    return n + !(a & 0x8000000000000000ULL);
}

uint64_t __divmoddi4(uint64_t a, uint64_t b, uint64_t* c) {
    if (b > a) // divisor > numerator?
    {
        if (c)
            *c = a; // remainder = numerator
        return 0;   // quotient = 0
    }
    if (!hi(b)) // divisor is 32-bit
    {
        if (b == 0) // divide by 0
        {
            volatile char x = 0;
            x = 1 / x; // force an exception
        }
        if (b == 1) // divide by 1
        {
            if (c)
                *c = 0; // remainder = 0
            return a;   // quotient = numerator
        }
        if (!hi(a)) // numerator is also 32-bit
        {
            if (c)
                *c = lo(a) % lo(b); // use generic 32-bit operators
            return lo(a) / lo(b);
        }
    }

    // let's do long division
    char bits = __clzdi2(b) - __clzdi2(a) + 1; // number of bits to iterate (a and b are non-zero)
    uint64_t rem = a >> bits;                  // init remainder
    a <<= 64 - bits;                           // shift numerator to the high bit
    uint64_t wrap = 0;                         // start with wrap = 0

    while (bits-- > 0) {              // for each bit
        rem = (rem << 1) | (a >> 63); // shift numerator MSB to remainder LSB
        a = (a << 1) | (wrap & 1);    // shift out the numerator, shift in wrap
        wrap = ((int64_t)(b - rem - 1) >> 63); // wrap = (b > rem) ? 0 : 0xffffffffffffffff (via sign extension)
        rem -= b & wrap;                       // if (wrap) rem -= b
    }

    if (c) {
        *c = rem; // maybe set remainder
    }

    return (a << 1) | (wrap & 1); // return the quotient
}

int64_t __divdi3(int64_t a, int64_t b) {
    uint64_t q = __divmoddi4(abs(a), abs(b), (void*) 0);

    return negate(q, a ^ b); // negate q if a and b signs are different
}

uint64_t __udivdi3(uint64_t a, uint64_t b) {
    return __divmoddi4(a, b, (void*) 0);
}

uint64_t __umoddi3(uint64_t a, uint64_t b) {
    uint64_t r;
    __divmoddi4(a, b, &r);
    return r;
}

#undef abs