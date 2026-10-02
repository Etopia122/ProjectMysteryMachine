#ifndef XMATH_H
#define XMATH_H

extern "C" {
double fmod(double, double);
double pow(double, double);
double atan2(double, double);
double cos(double);
double sin(double);
}

#define PI 3.1415927f

// Like MSL's inline sqrtf (math_ppc.h): three Newton-Raphson steps on
// frsqrte. Scooby's version uses plain double literals rather than static
// constants (the literals sit in each TU's .sdata2 pool).
inline float xsqrt(float x)
{
    volatile float y;
    if (x > 0.0f)
    {
        double guess = __frsqrte((double)x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        y = (float)(x * guess);
        return y;
    }
    return x;
}

inline float xpow(float x, float y)
{
    return pow(x, y);
}

inline float xatan2(float y, float x)
{
    return atan2(y, x);
}

inline float xfmod(float x, float y)
{
    return fmod(x, y);
}

inline float icos(float x)
{
    return cos(x);
}

inline float isin(float x)
{
    return sin(x);
}

void xMathInit();
void xMathExit();
unsigned int xMathSolveQuadratic(float a, float b, float c, float* x1, float* x2);
unsigned int xMathSolveCubic(float a, float b, float c, float d, float* x1, float* x2, float* x3);
float xAngleClamp(float a);
float xDangleClamp(float a);

#endif
