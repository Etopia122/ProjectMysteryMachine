#include "xMath3.h"
#include "rwtypes.h"


class iAnimSKBHeader;

float iAnimDurationSKB(iAnimSKBHeader* data);
void iAnimEvalSKB(iAnimSKBHeader* data, float time, unsigned int flags, _xVec3* tran, _xQuat* quat);

// RenderWare quaternion types (layouts as in RW 3.x)
struct RtQuat
{
    RwV3d imag;
    float real;
};

struct RtQuatSlerpCache
{
    RtQuat raFrom;
    RtQuat raTo;
    float omega;
    signed int nearlyZeroOm;
};

extern "C" void RtQuatSetupSlerpCache(RtQuat* qpFrom, RtQuat* qpTo, RtQuatSlerpCache* sCache);

// Taylor series for sin(x) on [-pi, pi], as in RenderWare's RwSinMinusPiToPiMacro
#define iAnimSinMinusPiToPi(result, x)                                                             \
    do                                                                                             \
    {                                                                                              \
        const float z = (x) * (x);                                                                 \
        const float v = z * (x);                                                                   \
        const float r = (0.008333333767950535f +                                                   \
                         z * (-0.00019841270113829523f +                                           \
                              z * (2.7557314297155244e-06f +                                       \
                                   z * (-2.5050759688838298e-08f + z * 1.5896910177115586e-10f)))); \
        (result) = (x) + v * (-0.1666666716337204f + z * r);                                       \
    } while (0)

// Mirrors RenderWare's RtQuatSlerpMacro
#define iAnimQuatSlerp(qpResult, qpFrom, qpTo, rT, sCache)                                         \
    do                                                                                             \
    {                                                                                              \
        if ((rT) <= 0.0f)                                                                          \
        {                                                                                          \
            *(qpResult) = *(qpFrom);                                                               \
        }                                                                                          \
        else if (1.0f <= (rT))                                                                     \
        {                                                                                          \
            *(qpResult) = *(qpTo);                                                                 \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            float rSclFrom = 1.0f - (rT);                                                          \
            float rSclTo = (rT);                                                                   \
                                                                                                   \
            if (!((sCache)->nearlyZeroOm))                                                         \
            {                                                                                      \
                rSclFrom *= (sCache)->omega;                                                       \
                iAnimSinMinusPiToPi(rSclFrom, rSclFrom);                                           \
                rSclTo *= (sCache)->omega;                                                         \
                iAnimSinMinusPiToPi(rSclTo, rSclTo);                                               \
            }                                                                                      \
                                                                                                   \
            (qpResult)->imag.x = (sCache)->raFrom.imag.x * rSclFrom;                               \
            (qpResult)->imag.y = (sCache)->raFrom.imag.y * rSclFrom;                               \
            (qpResult)->imag.z = (sCache)->raFrom.imag.z * rSclFrom;                               \
            (qpResult)->imag.x += (sCache)->raTo.imag.x * rSclTo;                                  \
            (qpResult)->imag.y += (sCache)->raTo.imag.y * rSclTo;                                  \
            (qpResult)->imag.z += (sCache)->raTo.imag.z * rSclTo;                                  \
            (qpResult)->real = ((sCache)->raFrom.real * rSclFrom) + ((sCache)->raTo.real * rSclTo); \
        }                                                                                          \
    } while (0)

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iAnimInit()
{
}

void iAnimEval(void* RawData, float time, unsigned int flags, _xVec3* tran, _xQuat* quat)
{
    iAnimEvalSKB((iAnimSKBHeader*)RawData, time, flags, tran, quat);
}

float iAnimDuration(void* RawData)
{
    return iAnimDurationSKB((iAnimSKBHeader*)RawData);
}

unsigned int iAnimBoneCount(void* RawData)
{
    return *(unsigned short*)((unsigned char*)RawData + 8);
}

void iAnimBlend(float BlendFactor, float BlendRecip, unsigned short* BlendTimeOffset,
                float* BoneTable, unsigned int BoneCount, _xVec3* Tran1, _xQuat* Quat1,
                _xVec3* Tran2, _xQuat* Quat2, _xVec3* TranDest, _xQuat* QuatDest)
{
    unsigned int i;
    unsigned int invert = 0;
    RtQuat* q2;
    RtQuat ident = { 0.0f, 0.0f, 0.0f, 1.0f };
    _xVec3* t2;

    if (!Quat2)
    {
        q2 = &ident;
        invert = 1;
        t2 = (_xVec3*)&ident.imag;
    }
    else
    {
        q2 = (RtQuat*)Quat2;
        t2 = Tran2;
    }

    if (BlendFactor < 0.0f)
    {
        BlendFactor = -BlendFactor;
        invert ^= 1;
    }

    if (!BoneTable && !BlendTimeOffset)
    {
        float lerp = BlendFactor * BlendRecip;

        if (lerp < 0.0f)
        {
            lerp = 0.0f;
        }
        else if (lerp > 1.0f)
        {
            lerp = 1.0f;
        }

        if (invert)
        {
            lerp = 1.0f - lerp;
        }

        if (Quat1)
        {
            for (i = 0; i < BoneCount; i++)
            {
                RtQuatSlerpCache qcache;

                RtQuatSetupSlerpCache((RtQuat*)Quat1, q2, &qcache);
                iAnimQuatSlerp((RtQuat*)QuatDest, (RtQuat*)Quat1, q2, lerp, &qcache);

                Quat1++;
                if (Quat2)
                {
                    q2++;
                }
                QuatDest++;
            }
        }

        if (Tran1)
        {
            if (Quat2)
            {
                for (i = 0; i < BoneCount; i++, TranDest++, t2++, Tran1++)
                {
                    TranDest->x = lerp * (t2->x - Tran1->x) + Tran1->x;
                    TranDest->y = lerp * (t2->y - Tran1->y) + Tran1->y;
                    TranDest->z = lerp * (t2->z - Tran1->z) + Tran1->z;
                }
            }
            else
            {
                for (i = 0; i < BoneCount; i++, TranDest++, Tran1++)
                {
                    TranDest->x = lerp * (t2->x - Tran1->x) + Tran1->x;
                    TranDest->y = lerp * (t2->y - Tran1->y) + Tran1->y;
                    TranDest->z = lerp * (t2->z - Tran1->z) + Tran1->z;
                }
            }
        }
    }
    else
    {
        float baselerp;

        if (!BlendTimeOffset)
        {
            baselerp = BlendFactor * BlendRecip;

            if (baselerp < 0.0f)
            {
                baselerp = 0.0f;
            }
            else if (baselerp > 1.0f)
            {
                baselerp = 1.0f;
            }

            if (invert)
            {
                baselerp = 1.0f - baselerp;
            }
        }

        for (i = 0; i < BoneCount; i++)
        {
            float lerp;

            if (BlendTimeOffset)
            {
                baselerp = -(1 / 1024.0f * BlendTimeOffset[i * 2] - BlendFactor);

                if (BlendTimeOffset[i * 2 + 1] != 0)
                {
                    baselerp *= 1 / 1024.0f * BlendTimeOffset[i * 2 + 1];

                    if (baselerp < 0.0f)
                    {
                        baselerp = 0.0f;
                    }
                    else if (baselerp > 1.0f)
                    {
                        baselerp = 1.0f;
                    }
                }
                else
                {
                    if (baselerp < 0.0f)
                    {
                        baselerp = 0.0f;
                    }
                    else
                    {
                        baselerp = 1.0f;
                    }
                }

                if (invert)
                {
                    baselerp = 1.0f - baselerp;
                }
            }

            if (BoneTable)
            {
                lerp = baselerp * BoneTable[i];
            }
            else
            {
                lerp = baselerp;
            }

            if (Quat1)
            {
                RtQuatSlerpCache qcache;

                RtQuatSetupSlerpCache((RtQuat*)Quat1, q2, &qcache);
                iAnimQuatSlerp((RtQuat*)QuatDest, (RtQuat*)Quat1, q2, lerp, &qcache);

                Quat1++;
                if (Quat2)
                {
                    q2++;
                }
                QuatDest++;
            }

            if (Tran1)
            {
                TranDest->x = lerp * (t2->x - Tran1->x) + Tran1->x;
                TranDest->y = lerp * (t2->y - Tran1->y) + Tran1->y;
                TranDest->z = lerp * (t2->z - Tran1->z) + Tran1->z;

                Tran1++;
                if (Quat2)
                {
                    t2++;
                }
                TranDest++;
            }
        }
    }
}
