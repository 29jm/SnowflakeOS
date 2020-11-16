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

double exp(double x) {
    double x0 = fabs(x);

    if (x == 0) {
        return 1;
    }

    const double ln2 = 0.6931471805599453;
    int k = ceil((x0 / ln2) - 0.5);
    double p = 1 << k;
    double r = x0 - (k * ln2);
    double tn = 1;

    for (int i = 14; i > 0; i--) {
        tn = tn * (r / i) + 1;
    }

    p *= tn;

    if (x < 0) {
        return 1 / p;
    }

    return p;
}

double log(double y) {
    int yi = y;
    int log2 = 0;
    double x, r;

    while (yi >>= 1) {
        log2++;
    }

    x = (double) (1 << log2);
    x = y / x;
    r = -1.7417939 + (2.8212026 + (-1.4699568 + (0.44717955 - 0.056570851 * x) * x) * x) * x;
    r += 0.69314718 * log2;

    return r;
}

double ceil(double x) {
    int n = (int) x;

    if (n >= x) {
        return n;
    }

    return n + 1;
}

double pow(double x, double y) {
    return exp(x*log(y));
}

int powi(int x, int y) {
    unsigned int n = 1;

    while (y--) {
        n *= x;
    }

    return n;
}

/* Note: not in the official <math.h>
 */
float clamp(float val, float min, float max) {
    return fminf(fmaxf(val, min), max);
}