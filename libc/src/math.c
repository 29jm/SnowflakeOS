#include <math.h>

double fmax(double a, double b) {
    return a > b ? a : b;
}

float fmaxf(float a, float b) {
    return a > b ? a : b;
}

double fmin(double a, double b) {
    return a < b ? a : b;
}

float fminf(float a, float b) {
    return a < b ? a : b;
}

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

double fabs(double x) {
    if (x < 0) {
        return -x;
    }

    return x;
}

/* Note: not in the official <math.h>
 */
float clamp(float val, float min, float max) {
    return fminf(fmaxf(val, min), max);
}