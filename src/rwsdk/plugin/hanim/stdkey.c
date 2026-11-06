/*
 * HAnim standard keyframes: a rotation quaternion and a translation per
 * keyframe. Interpolation and blending slerp the rotations (falling back to
 * a plain lerp for nearly equal ones) and lerp the translations.
 */

#include "rwsdk/rwcore.h"

typedef struct RtQuat {
    /* 0x00 */ RwV3d imag;
    /* 0x0C */ RwReal real;
} RtQuat;

typedef struct RpHAnimStdKeyFrame RpHAnimStdKeyFrame;

struct RpHAnimStdKeyFrame {
    /* 0x00 */ RpHAnimStdKeyFrame* prevFrame;
    /* 0x04 */ RwReal time;
    /* 0x08 */ RtQuat q;
    /* 0x18 */ RwV3d t;
};

typedef struct RpHAnimAnimation {
    /* 0x00 */ void* interpInfo;
    /* 0x04 */ RwInt32 numFrames;
    /* 0x08 */ RwInt32 flags;
    /* 0x0C */ RwReal duration;
    /* 0x10 */ void* pFrames;
    /* 0x14 */ void* customData;
} RpHAnimAnimation;

typedef union RpHAnimFloatBits {
    RwReal f;
    RwInt32 i;
} RpHAnimFloatBits;

#define rwMATRIXTYPEORTHONORMAL 0x00000003

RwReal _rwSqrt(RwReal num);

#define RtQuatDotProduct(q0, q1)                                                                       \
    ((q0)->imag.x * (q1)->imag.x + (q0)->imag.y * (q1)->imag.y + (q0)->imag.z * (q1)->imag.z +        \
     (q0)->real * (q1)->real)

#define RtQuatModulusSquared(q)                                                                        \
    ((q)->imag.x * (q)->imag.x + (q)->imag.y * (q)->imag.y + (q)->imag.z * (q)->imag.z +               \
     (q)->real * (q)->real)

/* o = q1 * q2 */
#define RtQuatMultiply(o, q1, q2)                                                                      \
    do {                                                                                               \
        (o)->real = (q1)->real * (q2)->real -                                                          \
                    ((q1)->imag.x * (q2)->imag.x + (q1)->imag.y * (q2)->imag.y + (q1)->imag.z * (q2)->imag.z); \
        (o)->imag.x = (q1)->imag.y * (q2)->imag.z - (q1)->imag.z * (q2)->imag.y;                       \
        (o)->imag.y = (q1)->imag.z * (q2)->imag.x - (q1)->imag.x * (q2)->imag.z;                       \
        (o)->imag.z = (q1)->imag.x * (q2)->imag.y - (q1)->imag.y * (q2)->imag.x;                       \
        (o)->imag.x += (q2)->imag.x * (q1)->real;                                                      \
        (o)->imag.y += (q2)->imag.y * (q1)->real;                                                      \
        (o)->imag.z += (q2)->imag.z * (q1)->real;                                                      \
        (o)->imag.x += (q1)->imag.x * (q2)->real;                                                      \
        (o)->imag.y += (q1)->imag.y * (q2)->real;                                                      \
        (o)->imag.z += (q1)->imag.z * (q2)->real;                                                      \
    } while (0)

static inline void RpHAnimACos(RwReal* result, RwReal x)
{
    RpHAnimFloatBits dfBits;
    RpHAnimFloatBits sBits;
    RpHAnimFloatBits bits;
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

static inline RwReal RpHAnimSin(RwReal x)
{
    RwReal x2 = x * x;

    return x + x2 * x *
                   (-0.166666672f +
                    x2 * (0.00833333377f +
                          x2 * (-0.000198412701f +
                                x2 * (2.75573143e-06f + x2 * (-2.50507597e-08f + 1.58969102e-10f * x2)))));
}

void RpHAnimStdKeyFrameToMatrix(RwMatrix* pMatrix, void* pVoidIFrame)
{
    RpHAnimStdKeyFrame* pIFrame = (RpHAnimStdKeyFrame*)pVoidIFrame;
    RwReal x = pIFrame->q.imag.x;
    RwReal y = pIFrame->q.imag.y;
    RwReal z = pIFrame->q.imag.z;
    RwReal w = pIFrame->q.real;
    RwV3d square;
    RwV3d cross;
    RwV3d wimag;

    square.x = x * x;
    square.y = y * y;
    square.z = z * z;
    cross.x = y * z;
    cross.y = z * x;
    cross.z = x * y;
    wimag.x = w * x;
    wimag.y = w * y;
    wimag.z = w * z;

    pMatrix->right.x = 1.0f - 2.0f * (square.y + square.z);
    pMatrix->right.y = 2.0f * (cross.z + wimag.z);
    pMatrix->right.z = 2.0f * (cross.y - wimag.y);
    pMatrix->up.x = 2.0f * (cross.z - wimag.z);
    pMatrix->up.y = 1.0f - 2.0f * (square.x + square.z);
    pMatrix->up.z = 2.0f * (cross.x + wimag.x);
    pMatrix->at.x = 2.0f * (cross.y + wimag.y);
    pMatrix->at.y = 2.0f * (cross.x - wimag.x);
    pMatrix->at.z = 1.0f - 2.0f * (square.x + square.y);
    pMatrix->pos.x = 0.0f;
    pMatrix->pos.y = 0.0f;
    pMatrix->pos.z = 0.0f;
    pMatrix->flags = rwMATRIXTYPEORTHONORMAL;

    pMatrix->pos.x = pIFrame->t.x;
    pMatrix->pos.y = pIFrame->t.y;
    pMatrix->pos.z = pIFrame->t.z;
}

void RpHAnimStdKeyFrameInterpolate(void* pVoidOut, void* pVoidIn1, void* pVoidIn2, RwReal time, void* customData)
{
    RwReal fDot;
    RwReal fScale;
    RpHAnimStdKeyFrame* pOut;
    RwReal fScale0;
    RpHAnimStdKeyFrame* pIn1;
    RpHAnimStdKeyFrame* pIn2;

    pOut = (RpHAnimStdKeyFrame*)pVoidOut;
    pIn1 = (RpHAnimStdKeyFrame*)pVoidIn1;
    pIn2 = (RpHAnimStdKeyFrame*)pVoidIn2;
    fScale = (time - pIn1->time) / (pIn2->time - pIn1->time);
    fDot = RtQuatDotProduct(&pIn1->q, &pIn2->q);

    pOut->t.x = pIn1->t.x + fScale * (pIn2->t.x - pIn1->t.x);
    pOut->t.y = pIn1->t.y + fScale * (pIn2->t.y - pIn1->t.y);
    pOut->t.z = pIn1->t.z + fScale * (pIn2->t.z - pIn1->t.z);
    pOut->time = time;

    if (fDot < 0.0f) {
        fDot = -fDot;
        pIn2->q.imag.x = -pIn2->q.imag.x;
        pIn2->q.imag.y = -pIn2->q.imag.y;
        pIn2->q.imag.z = -pIn2->q.imag.z;
        pIn2->q.real = -pIn2->q.real;
    }

    fScale0 = 1.0f - fScale;
    if (!(fDot >= 0.999000013f)) {
        RwReal invSinOmega;

        RpHAnimACos(&fDot, fDot);

        fScale0 *= fDot;
        fScale *= fDot;
        invSinOmega = 1.0f / RpHAnimSin(fDot);
        fScale0 = RpHAnimSin(fScale0) * invSinOmega;
        fScale = RpHAnimSin(fScale) * invSinOmega;
    }

    pOut->q.imag.x = fScale0 * pIn1->q.imag.x + fScale * pIn2->q.imag.x;
    pOut->q.imag.y = fScale0 * pIn1->q.imag.y + fScale * pIn2->q.imag.y;
    pOut->q.imag.z = fScale0 * pIn1->q.imag.z + fScale * pIn2->q.imag.z;
    pOut->q.real = fScale0 * pIn1->q.real + fScale * pIn2->q.real;
}

void RpHAnimStdKeyFrameBlend(void* pVoidOut, void* pVoidIn1, void* pVoidIn2, RwReal fAlpha)
{
    RpHAnimStdKeyFrame* pOut = (RpHAnimStdKeyFrame*)pVoidOut;
    RpHAnimStdKeyFrame* pIn1 = (RpHAnimStdKeyFrame*)pVoidIn1;
    RpHAnimStdKeyFrame* pIn2 = (RpHAnimStdKeyFrame*)pVoidIn2;
    RwReal fDot = RtQuatDotProduct(&pIn1->q, &pIn2->q);
    RwReal fScale0;

    pOut->t.x = pIn1->t.x + fAlpha * (pIn2->t.x - pIn1->t.x);
    pOut->t.y = pIn1->t.y + fAlpha * (pIn2->t.y - pIn1->t.y);
    pOut->t.z = pIn1->t.z + fAlpha * (pIn2->t.z - pIn1->t.z);

    if (fDot < 0.0f) {
        fDot = -fDot;
        pIn2->q.imag.x = -pIn2->q.imag.x;
        pIn2->q.imag.y = -pIn2->q.imag.y;
        pIn2->q.imag.z = -pIn2->q.imag.z;
        pIn2->q.real = -pIn2->q.real;
    }

    fScale0 = 1.0f - fAlpha;
    if (!(fDot >= 0.999000013f)) {
        RwReal invSinOmega;

        RpHAnimACos(&fDot, fDot);

        fScale0 *= fDot;
        fAlpha *= fDot;
        invSinOmega = 1.0f / RpHAnimSin(fDot);
        fScale0 = RpHAnimSin(fScale0) * invSinOmega;
        fAlpha = RpHAnimSin(fAlpha) * invSinOmega;
    }

    pOut->q.imag.x = fScale0 * pIn1->q.imag.x + fAlpha * pIn2->q.imag.x;
    pOut->q.imag.y = fScale0 * pIn1->q.imag.y + fAlpha * pIn2->q.imag.y;
    pOut->q.imag.z = fScale0 * pIn1->q.imag.z + fAlpha * pIn2->q.imag.z;
    pOut->q.real = fScale0 * pIn1->q.real + fAlpha * pIn2->q.real;
}

RpHAnimAnimation* RpHAnimStdKeyFrameStreamRead(RwStream* stream, RpHAnimAnimation* pAnimation)
{
    RpHAnimStdKeyFrame* pFrame;
    RpHAnimStdKeyFrame* pFrames;
    RwInt32 i;
    RwInt32 prevFrame;

    pAnimation->pFrames = RwMalloc(pAnimation->numFrames * sizeof(RpHAnimStdKeyFrame));
    pFrames = (RpHAnimStdKeyFrame*)pAnimation->pFrames;
    if (!pFrames) {
        return NULL;
    }

    pFrame = pFrames;
    for (i = 0; i < pAnimation->numFrames; i++) {
        if (!RwStreamReadReal(stream, &pFrame->time, sizeof(RwReal) * 8)) {
            return NULL;
        }
        if (!RwStreamReadInt32(stream, &prevFrame, sizeof(RwInt32))) {
            return NULL;
        }
        pFrame->prevFrame = &pFrames[(RwUInt32)prevFrame / sizeof(RpHAnimStdKeyFrame)];
        pFrame++;
    }
    return pAnimation;
}

RwBool RpHAnimStdKeyFrameStreamWrite(RpHAnimAnimation* pAnimation, RwStream* stream)
{
    RpHAnimStdKeyFrame* pFrame;
    RwInt32 i;
    RpHAnimStdKeyFrame* pFrames;
    RwInt32 prevFrame;

    pFrames = (RpHAnimStdKeyFrame*)pAnimation->pFrames;
    pFrame = pFrames;

    for (i = 0; i < pAnimation->numFrames; i++) {
        if (!RwStreamWriteReal(stream, &pFrame->time, sizeof(RwReal) * 8)) {
            return FALSE;
        }
        prevFrame = (RwUInt8*)pFrame->prevFrame - (RwUInt8*)pFrames;
        if (!RwStreamWriteInt32(stream, &prevFrame, sizeof(RwInt32))) {
            return FALSE;
        }
        pFrame++;
    }
    return TRUE;
}

RwInt32 RpHAnimStdKeyFrameStreamGetSize(RpHAnimAnimation* pAnimation)
{
    RwInt32 size = 0;
    RwInt32 i;

    for (i = 0; i < pAnimation->numFrames; i++) {
        size += sizeof(RpHAnimStdKeyFrame);
    }
    return size;
}

void RpHAnimStdKeyFrameMulRecip(void* pVoidFrame, void* pVoidStart)
{
    RpHAnimStdKeyFrame* pFrame = (RpHAnimStdKeyFrame*)pVoidFrame;
    RpHAnimStdKeyFrame* pStart = (RpHAnimStdKeyFrame*)pVoidStart;
    RtQuat qRecip;
    RtQuat qTmp;
    RwReal modulus = RtQuatModulusSquared(&pStart->q);

    if (modulus > 0.0f) {
        RwReal recip = 1.0f / modulus;

        qRecip.real = pStart->q.real * recip;
        qRecip.imag.x = pStart->q.imag.x * -recip;
        qRecip.imag.y = pStart->q.imag.y * -recip;
        qRecip.imag.z = pStart->q.imag.z * -recip;
    }

    qTmp = pFrame->q;
    RtQuatMultiply(&pFrame->q, &qRecip, &qTmp);

    pFrame->t.x = pFrame->t.x - pStart->t.x;
    pFrame->t.y = pFrame->t.y - pStart->t.y;
    pFrame->t.z = pFrame->t.z - pStart->t.z;
}

void RpHAnimStdKeyFrameAdd(void* pVoidOut, void* pVoidIn1, void* pVoidIn2)
{
    RpHAnimStdKeyFrame* pOut = (RpHAnimStdKeyFrame*)pVoidOut;
    RpHAnimStdKeyFrame* pIn1 = (RpHAnimStdKeyFrame*)pVoidIn1;
    RpHAnimStdKeyFrame* pIn2 = (RpHAnimStdKeyFrame*)pVoidIn2;

    RtQuatMultiply(&pOut->q, &pIn1->q, &pIn2->q);

    pOut->t.x = pIn1->t.x + pIn2->t.x;
    pOut->t.y = pIn1->t.y + pIn2->t.y;
    pOut->t.z = pIn1->t.z + pIn2->t.z;
}
