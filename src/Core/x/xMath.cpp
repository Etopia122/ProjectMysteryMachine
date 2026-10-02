#include "xMath.h"

void iMathInit();
void iMathExit();
void xMath3Init();
void xMath3Exit();

// Functions are in reverse DOL order (-inline deferred emits last to first).
// xMathSolveQuadratic and xAngleClamp are inlined into xMathSolveCubic.

void xMathInit()
{
    iMathInit();
    xMath3Init();
}

void xMathExit()
{
    xMath3Exit();
    iMathExit();
}

unsigned int xMathSolveQuadratic(float a, float b, float c, float* x1, float* x2)
{
    float d;
    float dx;
    float p;

    if (a == 0.0f)
    {
        if (b == 0.0f)
        {
            return 0;
        }
        *x1 = -c / b;
        return 1;
    }

    d = b * b - 4.0f * a * c;
    if (d < 0.0f)
    {
        return 0;
    }

    dx = 1.0f / (2.0f * a);
    *x1 = -b * dx;
    if (d == 0.0f)
    {
        return 1;
    }

    *x2 = *x1;
    p = dx * xsqrt(d);
    if (a > 0.0f)
    {
        *x1 -= p;
        *x2 += p;
    }
    else
    {
        *x1 += p;
        *x2 -= p;
    }
    return 2;
}

unsigned int xMathSolveCubic(float a, float b, float c, float d, float* x1, float* x2, float* x3)
{
    float arecip;
    float fA;
    float fB;
    float fOffset;
    float fDiscr;
    float fHalfB;
    float fDist;
    float fAngle;
    float fCos;
    float fSin;
    float t;
    float u;
    float v;

    if (a == 0.0f)
    {
        return xMathSolveQuadratic(b, c, d, x1, x2);
    }

    if (a != 1.0f)
    {
        arecip = 1.0f / a;
        b *= arecip;
        c *= arecip;
        d *= arecip;
    }

    fA = 0.33333334f * (3.0f * c - b * b);
    fB = 0.037037037f * (27.0f * d + (2.0f * (b * b * b) - 9.0f * b * c));
    fOffset = 0.33333334f * b;
    fDiscr = 0.25f * (fB * fB) + 0.037037037f * (fA * (fA * fA));
    fHalfB = 0.5f * fB;

    if ((float)__fabs(fDiscr) < 0.000001f)
    {
        fDiscr = 0.0f;
    }

    if (fDiscr > 0.0f)
    {
        t = xsqrt(fDiscr);
        u = -fHalfB + t;
        if (u >= 0.0f)
        {
            *x1 = xpow(u, 0.33333334f);
        }
        else
        {
            *x1 = -xpow(-u, 0.33333334f);
        }
        u = -fHalfB - t;
        if (u >= 0.0f)
        {
            *x1 += xpow(u, 0.33333334f);
        }
        else
        {
            *x1 -= xpow(-u, 0.33333334f);
        }
        *x1 -= fOffset;
        return 1;
    }

    if (fDiscr < 0.0f)
    {
        fDist = xsqrt(-0.33333334f * fA);
        fAngle = 0.33333334f * xAngleClamp(xatan2(xsqrt(-fDiscr), -fHalfB));
        fCos = icos(fAngle);
        fSin = isin(fAngle);
        *x1 = 2.0f * fDist * fCos - fOffset;
        *x2 = -fDist * (fCos + 1.7320508f * fSin) - fOffset;
        *x3 = -fDist * (fCos - 1.7320508f * fSin) - fOffset;
        return 3;
    }

    v = (fHalfB >= 0.0f) ? -xpow(fHalfB, 0.33333334f) : xpow(-fHalfB, 0.33333334f);
    *x1 = 2.0f * v - fOffset;
    *x2 = -v - fOffset;
    return 2;
}

float xAngleClamp(float a)
{
    float rem = xfmod(a, 2.0f * PI);
    if (rem < 0.0f)
    {
        rem += 2.0f * PI;
    }
    return rem;
}

float xDangleClamp(float a)
{
    float rem = xfmod(a, 2.0f * PI);
    if (rem >= PI)
    {
        rem -= 2.0f * PI;
    }
    else if (rem < -PI)
    {
        rem += 2.0f * PI;
    }
    return rem;
}
