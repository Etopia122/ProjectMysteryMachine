#include "xSpline.h"
#include "xMemMgr.h"

#ifndef NULL
#define NULL 0
#endif

extern "C" {
void* memcpy(void* dst, const void* src, unsigned int n);
void* malloc(unsigned int size);
void free(void* p);
double floor(double x);
double __frsqrte(double x);
extern unsigned long __float_nan[];
extern unsigned long __float_huge[];
}

static float sBasisBezier[4][4] = {
    { -1.0f, 3.0f, -3.0f, 1.0f },
    { 3.0f, -6.0f, 3.0f, 0.0f },
    { -3.0f, 3.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f, 0.0f },
};

static float sBasisHermite[4][4] = {
    { 2.0f, -3.0f, 0.0f, 1.0f },
    { 1.0f, -2.0f, 1.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f, 0.0f },
    { -2.0f, 3.0f, 0.0f, 0.0f },
};

// MSL's inline sqrt (math.h).
static inline double xSplineSqrt(double x)
{
    if (x > 0.0)
    {
        double guess = __frsqrte(x);
        guess = .5 * guess * (3.0 - guess * guess * x);
        guess = .5 * guess * (3.0 - guess * guess * x);
        guess = .5 * guess * (3.0 - guess * guess * x);
        guess = .5 * guess * (3.0 - guess * guess * x);
        return x * guess;
    }
    else if (x == 0)
    {
        return 0;
    }
    else if (x)
    {
        return *(float*)__float_nan;
    }
    return *(float*)__float_huge;
}

static inline void EvalCoef3(xCoef3* coef, float u, unsigned int deriv, _xVec3* o)
{
    switch (deriv)
    {
    case 0:
        o->x = u * (u * (coef->x.a[0] * u + coef->x.a[1]) + coef->x.a[2]) + coef->x.a[3];
        o->y = u * (u * (coef->y.a[0] * u + coef->y.a[1]) + coef->y.a[2]) + coef->y.a[3];
        o->z = u * (u * (coef->z.a[0] * u + coef->z.a[1]) + coef->z.a[2]) + coef->z.a[3];
        break;
    case 1:
        o->x = u * (2.0f * coef->x.a[1] + 3.0f * coef->x.a[0] * u) + coef->x.a[2];
        o->y = u * (2.0f * coef->y.a[1] + 3.0f * coef->y.a[0] * u) + coef->y.a[2];
        o->z = u * (2.0f * coef->z.a[1] + 3.0f * coef->z.a[0] * u) + coef->z.a[2];
        break;
    case 2:
        o->x = 2.0f * coef->x.a[1] + 6.0f * coef->x.a[0] * u;
        o->y = 2.0f * coef->y.a[1] + 6.0f * coef->y.a[0] * u;
        o->z = 2.0f * coef->z.a[1] + 6.0f * coef->z.a[0] * u;
        break;
    case 3:
        o->x = 6.0f * coef->x.a[0];
        o->y = 6.0f * coef->y.a[0];
        o->z = 6.0f * coef->z.a[0];
        break;
    default:
        o->x = 0.0f;
        o->y = 0.0f;
        o->z = 0.0f;
        break;
    }
}

static inline void BasisToCoef3(xCoef3* coef, float (*N)[4], _xVec3* v0, _xVec3* v1, _xVec3* v2,
                                _xVec3* v3)
{
    for (signed int i = 0; i < 4; i++)
    {
        coef->x.a[i] = v0->x * N[0][i] + N[1][i] * v1->x + N[2][i] * v2->x + N[3][i] * v3->x;
        coef->y.a[i] = v0->y * N[0][i] + N[1][i] * v1->y + N[2][i] * v2->y + N[3][i] * v3->y;
        coef->z.a[i] = v0->z * N[0][i] + N[1][i] * v1->z + N[2][i] * v2->z + N[3][i] * v3->z;
    }
}

static inline void CoefToUnity3(xCoef3* dest, xCoef3* coef, float t1, float t2)
{
    float dt = t2 - t1;
    float* o = dest->x.a;
    float* p = coef->x.a;
    for (unsigned int i = 0; i < 3; i++)
    {
        float a = p[0];
        float b = p[1];
        float c = p[2];
        float d = p[3];
        o[0] = dt * (dt * (a * dt));
        o[1] = t1 * (dt * (3.0f * a * dt)) + dt * (b * dt);
        o[2] = c * dt + (t1 * (t1 * (3.0f * a * dt)) + t1 * (2.0f * b * dt));
        o[3] = d + (c * t1 + (t1 * (t1 * (a * t1)) + t1 * (b * t1)));
        o += 4;
        p += 4;
    }
}

static inline void BasisBspline(float (*N)[4], float* t)
{
    unsigned int i;
    unsigned int k;
    unsigned int c;
    float d1;
    float d2;

    N[0][3] = 0.0f;
    N[1][3] = 0.0f;
    N[2][3] = 0.0f;
    N[3][3] = 1.0f;
    N[4][3] = 0.0f;
    N[5][3] = 0.0f;
    N[6][3] = 0.0f;
    for (i = 2; i <= 4; i++)
    {
        for (k = 0; k < 8 - i; k++)
        {
            float Ntemp[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            d1 = t[k + i - 1] - t[k];
            if (d1 != 0.0f)
            {
                d1 = 1.0f / d1;
            }
            d2 = t[k + i] - t[k + 1];
            if (d2 != 0.0f)
            {
                d2 = 1.0f / d2;
            }
            for (c = 5 - i; c <= 3; c++)
            {
                Ntemp[c - 1] += d1 * N[k][c] - d2 * N[k + 1][c];
                Ntemp[c] += d1 * (-t[k] * N[k][c]) + d2 * (t[k + i] * N[k + 1][c]);
            }
            N[k][0] = Ntemp[0];
            N[k][1] = Ntemp[1];
            N[k][2] = Ntemp[2];
            N[k][3] = Ntemp[3];
        }
    }
}

static inline void Interpolate_Bspline(_xVec3* data, _xVec3* control, float* knots,
                                       unsigned int nodata)
{
    float* alpha = (float*)malloc(nodata * sizeof(float));
    float* beta = (float*)malloc(nodata * sizeof(float));
    float* gamma = (float*)malloc(nodata * sizeof(float));

    alpha[nodata - 1] = 0.0f;
    alpha[0] = 0.0f;
    beta[nodata - 1] = 1.0f;
    beta[0] = 1.0f;
    gamma[nodata - 1] = 0.0f;
    gamma[0] = 0.0f;
    for (unsigned int i = 1; i < nodata - 1; i++)
    {
        float t1 = knots[i + 1];
        float t2 = knots[i + 2];
        float t3 = knots[i + 3];
        float t4 = knots[i + 4];
        float t5 = knots[i + 5];
        alpha[i] = ((t4 - t3) * (t4 - t3)) / (t4 - t1);
        beta[i] = ((t3 - t1) * (t4 - t3)) / (t4 - t1) + ((t5 - t3) * (t3 - t2)) / (t5 - t2);
        gamma[i] = ((t3 - t2) * (t3 - t2)) / (t5 - t2);
        alpha[i] /= t4 - t2;
        beta[i] /= t4 - t2;
        gamma[i] /= t4 - t2;
    }

    Tridiag_Solve(alpha, beta, gamma, data, control + 1, nodata);
    control[0] = control[1];
    control[nodata + 1] = control[nodata];
    free(alpha);
    free(beta);
    free(gamma);
}

static inline xSpline3* AllocSpline3(_xVec3* points, float* time, unsigned int numpoints,
                                     unsigned int numalloc, unsigned int flags, unsigned int type)
{
    xSpline3* spl = (xSpline3*)xMemAlloc(gActiveHeap, sizeof(xSpline3));
    if (numalloc < numpoints)
    {
        numalloc = numpoints;
    }
    spl->type = type;
    spl->flags = flags;
    spl->N = numpoints - 1;
    spl->allocN = numalloc - 1;
    spl->p12 = NULL;
    spl->bctrl = NULL;
    spl->knot = NULL;
    spl->coef = NULL;
    spl->arcSample = 0;
    spl->arcLength = NULL;
    spl->points = (_xVec3*)xMemAlloc(gActiveHeap, (spl->allocN + 1) * sizeof(_xVec3));
    memcpy(spl->points, points, (spl->N + 1) * sizeof(_xVec3));
    if (time != NULL)
    {
        spl->time = (float*)xMemAlloc(gActiveHeap, (spl->allocN + 1) * sizeof(float));
        memcpy(spl->time, time, (spl->N + 1) * sizeof(float));
    }
    else
    {
        spl->time = NULL;
    }
    return spl;
}

// TODO: NonMatching. .sdata2 starts with 3.0f, 2.0f (CoefToUnity3) and the sqrt
// doubles, as if out-of-line copies of those helpers were compiled first and later
// dead-stripped. The function diffs left are register/scheduling only.

// Functions are in reverse DOL order (-inline deferred emits last to first).

void Tridiag_Solve(float* a, float* b, float* c, _xVec3* d, _xVec3* x, signed int n)
{
    float* gamma = (float*)malloc(n * sizeof(float));
    _xVec3* delta = (_xVec3*)malloc(n * sizeof(_xVec3));
    signed int j;
    signed int k;

    gamma[0] = c[0] / b[0];
    for (k = 0; k < 3; k++)
    {
        (&delta[0].x)[k] = (&d[0].x)[k] / b[0];
    }
    for (j = 1; j < n; j++)
    {
        float beta = b[j] - a[j] * gamma[j - 1];
        gamma[j] = c[j] / beta;
        for (k = 0; k < 3; k++)
        {
            (&delta[j].x)[k] = ((&d[j].x)[k] - a[j] * ((float*)(delta + (j - 1)))[k]) / beta;
        }
    }
    for (k = 0; k < 3; k++)
    {
        ((float*)(x + (n - 1)))[k] = ((float*)(delta + (n - 1)))[k];
    }
    for (j = n - 2; j >= 0; j--)
    {
        for (k = 0; k < 3; k++)
        {
            (&x[j].x)[k] = (&delta[j].x)[k] - gamma[j] * ((float*)(x + (j + 1)))[k];
        }
    }
    free(gamma);
    free(delta);
}

float ArcLength3(xCoef3* coef, double ustart, double uend)
{
    double A;
    double B;
    double C;
    double D;
    double E;
    double h;
    double u;
    double sum;
    unsigned int i;
    double x0 = coef->x.a[0];
    double x1 = coef->x.a[1];
    double x2 = coef->x.a[2];
    double y0 = coef->y.a[0];
    double y1 = coef->y.a[1];
    double y2 = coef->y.a[2];
    double z0 = coef->z.a[0];
    double z1 = coef->z.a[1];
    double z2 = coef->z.a[2];

    double e0 = x0 * x0;
    double e1 = y0 * y0;
    double e2 = z0 * z0;
    E = 9.0 * (e0 + e1 + e2);
    D = 12.0 * (x0 * x1 + y0 * y1 + z0 * z1);
    C = 6.0 * (x0 * x2 + y0 * y2 + z0 * z2) + 4.0 * (x1 * x1 + y1 * y1 + z1 * z1);
    B = 4.0 * (x1 * x2 + y1 * y2 + z1 * z2);
    A = x2 * x2 + y2 * y2 + z2 * z2;
    h = (uend - ustart) / 50.0;
    u = ustart + h;
    sum = 0.0;
    for (i = 2; i < 51; i++)
    {
        if (!(i & 1))
        {
            sum += 4.0 * xSplineSqrt(A + u * (B + u * (C + u * (D + u * E))));
        }
        else
        {
            sum += 2.0 * xSplineSqrt(A + u * (B + u * (C + u * (D + u * E))));
        }
        u += h;
    }
    return (h * (sum + xSplineSqrt(A + ustart * (B + ustart * (C + ustart * (D + ustart * E)))) +
                 xSplineSqrt(A + uend * (B + uend * (C + uend * (D + uend * E)))))) /
           3.0;
}

void EvalBspline3(xSpline3* spl, float u, unsigned int deriv, _xVec3* o)
{
    float N[7][4];
    xCoef3 coef;
    unsigned int min;
    unsigned int max;
    unsigned int mid;

    if (u < 0.0f)
    {
        u = 0.0f;
    }
    if (u > spl->knot[spl->N + 3])
    {
        u = spl->knot[spl->N + 3];
    }

    min = 3;
    max = spl->N + 3;
    while (min + 1 != max)
    {
        mid = (min + max) >> 1;
        if (spl->knot[mid] >= u)
        {
            max = mid;
        }
        else
        {
            min = mid;
        }
    }

    signed int seg = min - 3;
    BasisBspline(N, &spl->knot[seg]);
    BasisToCoef3(&coef, N, spl->bctrl + seg, spl->bctrl + seg + 1, spl->bctrl + seg + 2,
                 spl->bctrl + seg + 3);
    EvalCoef3(&coef, u, deriv, o);
}

xCoef3* CoefSeg3(xSpline3* spl, unsigned int seg, xCoef3* tempCoef)
{
    switch (spl->type)
    {
    case 1:
        return &spl->coef[seg];
    case 2:
        BasisToCoef3(tempCoef, sBasisHermite, spl->points + seg, spl->p12 + seg * 2,
                     spl->p12 + seg * 2 + 1, spl->points + seg + 1);
        break;
    case 3:
        BasisToCoef3(tempCoef, sBasisBezier, spl->points + seg, spl->p12 + seg * 2,
                     spl->p12 + seg * 2 + 1, spl->points + seg + 1);
        break;
    case 4:
    {
        float N[7][4];
        BasisBspline(N, &spl->knot[seg]);
        BasisToCoef3(tempCoef, N, spl->bctrl + seg, spl->bctrl + seg + 1, spl->bctrl + seg + 2,
                     spl->bctrl + seg + 3);
        CoefToUnity3(tempCoef, tempCoef, spl->knot[seg + 3], spl->knot[seg + 4]);
        break;
    }
    }
    return tempCoef;
}

void xSpline3_EvalSeg(xSpline3* spl, float u, unsigned int deriv, _xVec3* o)
{
    xCoef3 tempCoef;
    unsigned int seg;
    float flr;

    if (spl->type == 4)
    {
        EvalBspline3(spl, u, deriv, o);
        return;
    }

    if (u < 0.0f)
    {
        u = 0.0f;
    }
    flr = floor(u);
    seg = flr;
    if (seg >= spl->N)
    {
        u = 1.0f;
        seg = spl->N - 1;
    }
    else
    {
        u = u - flr;
    }

    switch (spl->type)
    {
    case 1:
        EvalCoef3(&spl->coef[seg], u, deriv, o);
        return;
    case 2:
        BasisToCoef3(&tempCoef, sBasisHermite, spl->points + seg, spl->p12 + seg * 2,
                     spl->p12 + seg * 2 + 1, spl->points + seg + 1);
        EvalCoef3(&tempCoef, u, deriv, o);
        return;
    case 3:
        BasisToCoef3(&tempCoef, sBasisBezier, spl->points + seg, spl->p12 + seg * 2,
                     spl->p12 + seg * 2 + 1, spl->points + seg + 1);
        EvalCoef3(&tempCoef, u, deriv, o);
        return;
    }
}

float ArcEvalIterate(xSpline3* spl, float s, unsigned int deriv, _xVec3* o, unsigned int iterations)
{
    xCoef3 tempCoef;
    signed int min;
    signed int max;
    signed int test;
    signed int seg;
    signed int segmul;
    float umin;
    float umax;
    float smin;
    float smax;
    float segstart;
    float u;

    min = -1;
    max = spl->arcSample * spl->N - 1;
    while (min + 1 != max)
    {
        test = (min + max) >> 1;
        if (s > spl->arcLength[test])
        {
            min = test;
        }
        else
        {
            max = test;
        }
    }

    seg = max / (signed int)spl->arcSample;
    segmul = seg * spl->arcSample;
    umin = (float)(max % (signed int)spl->arcSample) / (float)spl->arcSample;
    umax = (float)(max + 1 - segmul) / (float)spl->arcSample;
    smin = max - 1 >= 0 ? spl->arcLength[max - 1] : 0.0f;
    smax = spl->arcLength[max];
    segstart = segmul - 1 >= 0 ? spl->arcLength[segmul - 1] : 0.0f;
    xCoef3* coef = CoefSeg3(spl, seg, &tempCoef);

    if (s <= smin)
    {
        EvalCoef3(coef, umin, deriv, o);
        return (float)seg + umin;
    }
    if (s >= smax)
    {
        EvalCoef3(coef, umax, deriv, o);
        return (float)seg + umax;
    }

    s -= segstart;
    smin -= segstart;
    smax -= segstart;
    while (iterations != 0)
    {
        float utest = 0.5f * (umax - umin) + umin;
        float arctest = ArcLength3(coef, 0.0, utest);
        if (s > arctest)
        {
            umin = utest;
            smin = arctest;
        }
        else
        {
            umax = utest;
            smax = arctest;
        }
        iterations--;
    }

    if (smax - smin == 0.0f)
    {
        u = umin;
    }
    else
    {
        u = umin + ((umax - umin) * (s - smin)) / (smax - smin);
        if (u < 0.0f)
        {
            u = 0.0f;
        }
        else if (u > 1.0f)
        {
            u = 1.0f;
        }
    }
    EvalCoef3(coef, u, deriv, o);
    return (float)seg + u;
}

float xSpline3_EvalArcApprox(xSpline3* spl, float s, unsigned int deriv, _xVec3* o)
{
    if (spl->arcLength != NULL)
    {
        return ArcEvalIterate(spl, s, deriv, o, 0);
    }
    xSpline3_EvalSeg(spl, s, deriv, o);
    return s;
}

void xSpline3_ArcInit(xSpline3* spl, unsigned int sample)
{
    xCoef3 tempCoef;
    float arcsum;
    float len = 0.0f;
    unsigned int seg;
    unsigned int i;

    if (sample < 1)
    {
        sample = 1;
    }
    spl->arcSample = sample;
    i = spl->arcLength != NULL ? spl->allocN * spl->arcSample : 0;
    if (i < spl->N * sample)
    {
        spl->arcLength = (float*)xMemAlloc(gActiveHeap, spl->arcSample * spl->allocN * sizeof(float));
    }

    arcsum = 0.0f;
    for (i = 0; i < spl->N; i++)
    {
        xCoef3* coef = CoefSeg3(spl, i, &tempCoef);
        for (seg = 0; seg < sample; seg++)
        {
            len = ArcLength3(coef, 0.0, (float)(seg + 1) / (float)sample);
            spl->arcLength[seg + i * sample] = arcsum + len;
        }
        arcsum += len;
    }
}

xSpline3* xSpline3_Bezier(_xVec3* points, float* time, unsigned int numpoints,
                          unsigned int numalloc, _xVec3* p1, _xVec3* p2)
{
    xSpline3* spl = AllocSpline3(points, time, numpoints, numalloc, 0, 3);
    spl->p12 = (_xVec3*)xMemAlloc(gActiveHeap, spl->allocN * 2 * sizeof(_xVec3));
    if (p1 == NULL || p2 == NULL)
    {
        xSpline3_Catmullize(spl);
    }
    else
    {
        for (unsigned int i = 0; i < spl->N; i++)
        {
            spl->p12[i * 2] = p1[i];
            spl->p12[i * 2 + 1] = p2[i];
        }
    }
    return spl;
}

void xSpline3_Update(xSpline3* spl)
{
    if (spl->type == 4)
    {
        Interpolate_Bspline(spl->points, spl->bctrl, spl->knot, spl->N + 1);
    }
    if (spl->arcLength != NULL)
    {
        xSpline3_ArcInit(spl, spl->arcSample);
    }
}

void xSpline3_Catmullize(xSpline3* spl)
{
    xSpline3_Update(spl);
}
