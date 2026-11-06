/*
 * Quaternion toolkit: precomputes the angle between two keyframe rotations
 * and the 1/sin(omega) scale a spherical interpolation between them needs.
 * The arc cosine is a single precision port of the usual rational
 * approximation, and the sine a Taylor polynomial.
 */

#include "rwsdk/rwcore.h"

typedef struct RtQuat {
    /* 0x00 */ RwV3d imag;
    /* 0x0C */ RwReal real;
} RtQuat;

typedef struct RtQuatSlerpCache {
    /* 0x00 */ RtQuat raFrom;
    /* 0x10 */ RtQuat raTo;
    /* 0x20 */ RwReal omega;
    /* 0x24 */ RwBool nearlyZeroOm;
} RtQuatSlerpCache;

typedef union RtQuatFloatBits {
    RwReal f;
    RwInt32 i;
} RtQuatFloatBits;

RwReal _rwSqrt(RwReal num);

#define RtQuatDotProduct(q0, q1)                                                                       \
    ((q0)->imag.x * (q1)->imag.x + (q0)->imag.y * (q1)->imag.y + (q0)->imag.z * (q1)->imag.z +        \
     (q0)->real * (q1)->real)

#define RtQuatNegate(o, i)                                                                             \
    do {                                                                                               \
        (o)->real = -(i)->real;                                                                        \
        (o)->imag.x = -(i)->imag.x;                                                                    \
        (o)->imag.y = -(i)->imag.y;                                                                    \
        (o)->imag.z = -(i)->imag.z;                                                                    \
    } while (0)

#define RtQuatScale(o, i, s)                                                                           \
    do {                                                                                               \
        (o)->real = (i)->real * (s);                                                                   \
        (o)->imag.x = (i)->imag.x * (s);                                                               \
        (o)->imag.y = (i)->imag.y * (s);                                                               \
        (o)->imag.z = (i)->imag.z * (s);                                                               \
    } while (0)

/*
 * Stand-in for the toolkit functions ahead of this one that the game never
 * calls (the linker strips them); they create the shared 1, 0 and 2 pool
 * constants first.
 */
void RtQuatPoolStandIn(RwReal* out)
{
    out[0] = 1.0f;
    out[1] = 0.0f;
    out[2] = 2.0f;
}

static inline void RtQuatACos(RwReal* result, RwReal x)
{
    RtQuatFloatBits dfBits;
    RtQuatFloatBits sBits;
    RtQuatFloatBits bits;
    RwInt32 hx;
    RwInt32 ix;
    RwReal z;
    RwReal p;
    RwReal q;
    RwReal r;
    RwReal w;
    RwReal s;

    bits.f = x;
    hx = bits.i;
    ix = hx & 0x7FFFFFFF;
    if (ix >= 0x3F800000) {
        if (hx > 0) {
            *result = 0.0f;
        return;
        }
        *result = 3.14159274f;
        return;
    }
    if (ix < 0x3F000000) {
        if (ix <= 0x23000000) {
            *result = 1.57079637f;
        return;
        }
        z = x * x;
        p = z * (0.166666672f +
                 z * (-0.325565815f +
                      z * (0.201212525f + z * (-0.0400555357f + z * (0.000791535014f + z * 3.47933092e-05f)))));
        q = 1.0f + z * (-2.40339494f + z * (2.02094579f + z * (-0.68828398f + z * 0.077038154f)));
        r = p / q;
        *result = 1.57079625f - (x - (7.54978942e-08f - x * r));
        return;
    }
    if (hx < 0) {
        z = (1.0f + x) * 0.5f;
        p = z * (0.166666672f +
                 z * (-0.325565815f +
                      z * (0.201212525f + z * (-0.0400555357f + z * (0.000791535014f + z * 3.47933092e-05f)))));
        q = 1.0f + z * (-2.40339494f + z * (2.02094579f + z * (-0.68828398f + z * 0.077038154f)));
        s = _rwSqrt(z);
        r = p / q;
        w = r * s - 7.54978942e-08f;
        *result = 3.1415925f - 2.0f * (s + w);
        return;
    } else {
        RwReal df;
        RwReal c;

        z = (1.0f - x) * 0.5f;
        s = _rwSqrt(z);
        sBits.f = s;
        dfBits.i = sBits.i & 0xFFFFF000;
        df = dfBits.f;
        c = (z - df * df) / (s + df);
        p = z * (0.166666672f +
                 z * (-0.325565815f +
                      z * (0.201212525f + z * (-0.0400555357f + z * (0.000791535014f + z * 3.47933092e-05f)))));
        q = 1.0f + z * (-2.40339494f + z * (2.02094579f + z * (-0.68828398f + z * 0.077038154f)));
        r = p / q;
        w = r * s + c;
        *result = 2.0f * (df + w);
        return;
    }
}

void RtQuatSetupSlerpCache(RtQuat* qpFrom, RtQuat* qpTo, RtQuatSlerpCache* sCache)
{
    RwReal cosTheta;

    sCache->raFrom = *qpFrom;
    cosTheta = RtQuatDotProduct(qpFrom, qpTo);
    if (cosTheta < 0.0f) {
        cosTheta = (cosTheta < -1.0f) ? 1.0f : -cosTheta;
        RtQuatNegate(&sCache->raTo, qpTo);
    } else {
        cosTheta = (cosTheta > 1.0f) ? 1.0f : cosTheta;
        sCache->raTo = *qpTo;
    }

    RtQuatACos(&sCache->omega, cosTheta);
    sCache->nearlyZeroOm = (cosTheta >= 0.999989986f);
    if (!sCache->nearlyZeroOm) {
        RwReal omega = sCache->omega;
        RwReal omega2 = omega * omega;
        RwReal invSinOmega =
            1.0f /
            (omega + omega2 * omega *
                         (-0.166666672f +
                          omega2 * (0.00833333377f +
                                    omega2 * (-0.000198412701f +
                                              omega2 * (2.75573143e-06f +
                                                        omega2 * (-2.50507597e-08f + 1.58969102e-10f * omega2))))));

        RtQuatScale(&sCache->raFrom, &sCache->raFrom, invSinOmega);
        RtQuatScale(&sCache->raTo, &sCache->raTo, invSinOmega);
    }
}
