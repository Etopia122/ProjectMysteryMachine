#include "xMath3.h"
#include "xMemMgr.h"

extern "C" void* memcpy(void* dst, const void* src, unsigned int n);


class iAnimSKBHeader
{
public:
    unsigned int Magic;       // 0x0
    unsigned int Flags;       // 0x4
    unsigned short BoneCount; // 0x8
    unsigned short TimeCount; // 0xA
    unsigned int KeyCount;    // 0xC
    float Scale[3];           // 0x10
};

class iAnimSKBKey
{
public:
    unsigned short TimeIndex; // 0x0
    signed short Quat[4];     // 0x2
    signed short Tran[3];     // 0xA
};

class RwV3d
{
public:
    float x;
    float y;
    float z;
};

class RtQuat
{
public:
    RwV3d imag;
    float real;
};

class RtQuatSlerpCache
{
public:
    RtQuat raFrom;            // 0x0
    RtQuat raTo;              // 0x10
    float omega;              // 0x20
    signed int nearlyEqual;   // 0x24
};

extern "C" void RtQuatSetupSlerpCache(RtQuat* qpFrom, RtQuat* qpTo, RtQuatSlerpCache* sCache);

extern unsigned char* giAnimScratch;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline float iAnimSKBSin(float x)
{
    float x2 = x * x;
    return x + x2 * x *
                   (x2 * (x2 * (x2 * (x2 * (1.589691e-10f * x2 + -2.505076e-08f) + 2.7557314e-06f) +
                                -0.0001984127f) +
                          0.008333334f) +
                    -0.16666667f);
}

static inline float iAnimSKBLerp(float a, float b, float t)
{
    return t * (b - a) + a;
}

static inline void iAnimSKBSlerp(RtQuat* result, RtQuat* from, RtQuat* to, float t,
                                 RtQuatSlerpCache* cache)
{
    if (t <= 0.0f)
    {
        *result = *from;
    }
    else if (1.0f <= t)
    {
        *result = *to;
    }
    else
    {
        float w1 = t;
        float w0 = 1.0f - t;

        if (!cache->nearlyEqual)
        {
            w0 *= cache->omega;
            w1 *= cache->omega;
            w0 = iAnimSKBSin(w0);
            w1 = iAnimSKBSin(w1);
        }

        result->imag.x = cache->raFrom.imag.x * w0;
        result->imag.y = cache->raFrom.imag.y * w0;
        result->imag.z = cache->raFrom.imag.z * w0;
        result->imag.x += cache->raTo.imag.x * w1;
        result->imag.y += cache->raTo.imag.y * w1;
        result->imag.z += cache->raTo.imag.z * w1;
        result->real = cache->raFrom.real * w0 + cache->raTo.real * w1;
    }
}

void iAnimEvalSKB(iAnimSKBHeader* data, float time, unsigned int flags, _xVec3* tran, _xQuat* quat)
{
    unsigned int i;
    unsigned int tidx;
    unsigned int bcount;
    unsigned int tcount;
    iAnimSKBKey* keys;
    float* times;
    unsigned short* offsets;

    tcount = data->TimeCount;
    bcount = data->BoneCount;
    keys = (iAnimSKBKey*)(data + 1);
    times = (float*)(keys + data->KeyCount);
    offsets = (unsigned short*)(times + tcount);

    if (time < 0.0f)
    {
        time = 0.0f;
    }
    if (time > times[tcount - 1])
    {
        time = times[tcount - 1];
    }

    tidx = (tcount - 1) % 4;
    while (times[tidx] < time)
    {
        tidx += 4;
    }
    while (tidx && time <= times[tidx])
    {
        tidx--;
    }

    offsets += tidx * bcount;

    if (flags & 0x1)
    {
        bcount = 1;
    }
    if (flags & 0x2)
    {
        bcount--;
        offsets++;
    }

    float scalex = data->Scale[0];
    float scaley = data->Scale[1];
    float scalez = data->Scale[2];

    for (i = 0; i < bcount; i++)
    {
        RtQuatSlerpCache qcache;
        RtQuat q1;
        RtQuat q2;
        iAnimSKBKey* k = &keys[*offsets];
        offsets++;

        float time1 = time - times[k->TimeIndex];
        float time2 = times[k[1].TimeIndex] - times[k->TimeIndex];
        float lerp = time1 / time2;

        q1.imag.x = (1.0f / 32767.0f) * k[0].Quat[0];
        q1.imag.y = (1.0f / 32767.0f) * k[0].Quat[1];
        q1.imag.z = (1.0f / 32767.0f) * k[0].Quat[2];
        q1.real = (1.0f / 32767.0f) * k[0].Quat[3];

        q2.imag.x = (1.0f / 32767.0f) * k[1].Quat[0];
        q2.imag.y = (1.0f / 32767.0f) * k[1].Quat[1];
        q2.imag.z = (1.0f / 32767.0f) * k[1].Quat[2];
        q2.real = (1.0f / 32767.0f) * k[1].Quat[3];

        RtQuatSetupSlerpCache(&q1, &q2, &qcache);
        iAnimSKBSlerp((RtQuat*)quat, &q1, &q2, lerp, &qcache);
        quat++;

        float x = iAnimSKBLerp(scalex * k[0].Tran[0], scalex * k[1].Tran[0], lerp);
        float y = iAnimSKBLerp(scaley * k[0].Tran[1], scaley * k[1].Tran[1], lerp);
        float z = iAnimSKBLerp(scalez * k[0].Tran[2], scalez * k[1].Tran[2], lerp);
        tran->x = x;
        tran->y = y;
        tran->z = z;
        tran++;
    }
}

float iAnimDurationSKB(iAnimSKBHeader* data);

void _iAnimSKBFixTransPop(void* data, void* pose, unsigned int maxbone)
{
    unsigned int boneIndex;
    iAnimSKBHeader* hb;
    _xQuat* q_a;
    _xVec3* t_a;
    _xQuat* q_b;
    _xVec3* t_b;
    iAnimSKBHeader* ha;
    iAnimSKBKey* ka;
    float* ta;
    unsigned int i;
    float invDuration;

    q_a = (_xQuat*)giAnimScratch;
    ha = (iAnimSKBHeader*)data;
    hb = (iAnimSKBHeader*)pose;
    q_b = (_xQuat*)((unsigned char*)q_a + 0x720);
    ka = (iAnimSKBKey*)(ha + 1);
    t_a = (_xVec3*)((unsigned char*)q_a + 0x400);
    t_b = (_xVec3*)((unsigned char*)q_b + 0x400);
    ta = (float*)(ka + ha->KeyCount);
    boneIndex = 0;

    if (!maxbone)
    {
        maxbone = ha->BoneCount;
    }

    float scalex = (hb->Scale[0] > ha->Scale[0]) ? hb->Scale[0] : ha->Scale[0];
    float scaley = (hb->Scale[1] > ha->Scale[1]) ? hb->Scale[1] : ha->Scale[1];
    float scalez = (hb->Scale[2] > ha->Scale[2]) ? hb->Scale[2] : ha->Scale[2];

    iAnimEvalSKB(ha, iAnimDurationSKB(ha), 0, t_a, q_a);
    iAnimEvalSKB(hb, 0.0f, 0, t_b, q_b);

    for (i = 0; i < ha->BoneCount; i++)
    {
        t_a[i].x = t_b[i].x - t_a[i].x;
        t_a[i].y = t_b[i].y - t_a[i].y;
        t_a[i].z = t_b[i].z - t_a[i].z;
    }

    invDuration = 1.0f / iAnimDurationSKB(ha);

    for (i = 0; i < ha->KeyCount; i++)
    {
        if (i != 0 && ka[i].TimeIndex < ka[i - 1].TimeIndex)
        {
            boneIndex++;
        }

        float x = ha->Scale[0] * ka[i].Tran[0];
        float y = ha->Scale[1] * ka[i].Tran[1];
        float z = ha->Scale[2] * ka[i].Tran[2];

        if (boneIndex < maxbone)
        {
            float t = ta[ka[i].TimeIndex];
            x += invDuration * (t_a[boneIndex].x * t);
            y += invDuration * (t_a[boneIndex].y * t);
            z += invDuration * (t_a[boneIndex].z * t);
        }

        ka[i].Tran[0] = x / scalex;
        ka[i].Tran[1] = y / scaley;
        ka[i].Tran[2] = z / scalez;
    }

    ha->Scale[0] = scalex;
    ha->Scale[1] = scaley;
    ha->Scale[2] = scalez;
}

#pragma auto_inline on
float iAnimDurationSKB(iAnimSKBHeader* data)
{
    return ((float*)((iAnimSKBKey*)(data + 1) + data->KeyCount))[data->TimeCount - 1];
}
#pragma auto_inline off

void _iAnimSKBSwapTranslate(iAnimSKBHeader* data, unsigned int bonelo, unsigned int bonehi)
{
    unsigned int lofirst;
    unsigned int i;
    unsigned int lolast;
    unsigned int bcount;
    unsigned int tcount;
    iAnimSKBKey* buflo;
    iAnimSKBKey* bufhi;
    unsigned int j;
    iAnimSKBKey* bufmid;
    iAnimSKBKey* keys;
    unsigned short* offsets;
    unsigned int locount;
    signed int out;
    unsigned int hilast;
    unsigned int hicount;
    unsigned int hifirst;
    unsigned int idx;
    unsigned short keyIndex;

    keys = (iAnimSKBKey*)(data + 1);
    tcount = data->TimeCount;
    bcount = data->BoneCount;
    offsets = (unsigned short*)((float*)(keys + data->KeyCount) + tcount);
    buflo = (iAnimSKBKey*)xMemPushTemp(tcount * 3 * sizeof(iAnimSKBKey));
    bufhi = buflo + tcount;
    bufmid = bufhi + tcount;

    lofirst = offsets[bonelo];
    lolast = offsets[(tcount - 2) * bcount + bonelo] + 1;
    locount = (lolast - lofirst) + 1;
    memcpy(buflo, &keys[lofirst], locount * sizeof(iAnimSKBKey));

    hifirst = offsets[bonehi];
    hilast = offsets[(tcount - 2) * bcount + bonehi] + 1;
    hicount = (hilast - hifirst) + 1;
    memcpy(bufhi, &keys[hifirst], hicount * sizeof(iAnimSKBKey));

    out = (signed int)((hifirst - 1) - (lolast + 1)) + 1;
    if (out > 0)
    {
        memcpy(bufmid, &keys[lolast + 1], out * sizeof(iAnimSKBKey));
    }

    memcpy(&keys[lofirst], bufhi, hicount * sizeof(iAnimSKBKey));
    idx = lofirst + hicount;
    if (out > 0)
    {
        memcpy(&keys[idx], bufmid, out * sizeof(iAnimSKBKey));
        idx += out;
    }
    memcpy(&keys[idx], buflo, locount * sizeof(iAnimSKBKey));

    xMemPopTemp(buflo);

    keyIndex = 0;
    for (i = 0; i < bcount; i++)
    {
        for (j = 0; j < tcount - 1; j++)
        {
            while (keys[keyIndex + 1].TimeIndex <= j)
            {
                keyIndex++;
            }
            offsets[j * bcount + i] = keyIndex;
        }

        while (keys[keyIndex].TimeIndex != tcount - 1)
        {
            keyIndex++;
        }
        keyIndex++;
    }
}

void _iAnimSKBAdjustTranslate(iAnimSKBHeader* data, unsigned int bone, float* tran)
{
    float outScale[3];
    float factor[3];
    float oldmax[3] = { 0.0f, 0.0f, 0.0f };
    float newmax[3] = { 0.0f, 0.0f, 0.0f };
    unsigned int tcount;
    unsigned int keyfirst;
    signed int ipos;
    unsigned int kcount;
    unsigned short* offsets;
    unsigned int idx;
    unsigned int keylast;
    unsigned int i;
    float pos;
    iAnimSKBKey* keys;

    tcount = data->TimeCount;
    kcount = data->KeyCount;
    keys = (iAnimSKBKey*)(data + 1);
    offsets = (unsigned short*)((float*)(keys + kcount) + tcount);
    keyfirst = offsets[bone];
    keylast = offsets[(tcount - 2) * data->BoneCount + bone] + 1;

    for (i = 0; i < kcount; i++)
    {
        for (idx = 0; idx < 3; idx++)
        {
            if (tran[idx])
            {
                pos = data->Scale[idx] * keys[i].Tran[idx];
                if ((float)__fabs(pos) > oldmax[idx])
                {
                    oldmax[idx] = __fabs(pos);
                }
                if (i >= keyfirst && i <= keylast)
                {
                    pos += tran[idx];
                    if ((float)__fabs(pos) > newmax[idx])
                    {
                        newmax[idx] = __fabs(pos);
                    }
                }
            }
        }
    }

    for (idx = 0; idx < 3; idx++)
    {
        if (tran[idx])
        {
            if (newmax[idx] > oldmax[idx])
            {
                outScale[idx] = data->Scale[idx] * newmax[idx] / oldmax[idx];
            }
            else
            {
                outScale[idx] = data->Scale[idx];
            }
            factor[idx] = 1.0f / outScale[idx];
        }
    }

    for (i = 0; i < kcount; i++)
    {
        for (idx = 0; idx < 3; idx++)
        {
            if (tran[idx])
            {
                if (i >= keyfirst && i <= keylast)
                {
                    ipos = factor[idx] * (data->Scale[idx] * keys[i].Tran[idx] + tran[idx]);
                    if (ipos < -32767)
                    {
                        ipos = -32767;
                    }
                    else if (ipos > 32767)
                    {
                        ipos = 32767;
                    }
                    keys[i].Tran[idx] = ipos;
                }
                else if (data->Scale[idx] != outScale[idx])
                {
                    ipos = data->Scale[idx] * keys[i].Tran[idx] * factor[idx];
                    if (ipos < -32767)
                    {
                        ipos = -32767;
                    }
                    else if (ipos > 32767)
                    {
                        ipos = 32767;
                    }
                    keys[i].Tran[idx] = ipos;
                }
            }
        }
    }

    for (idx = 0; idx < 3; idx++)
    {
        if (tran[idx])
        {
            data->Scale[idx] = outScale[idx];
        }
    }
}

void _iAnimSKBFlip180(iAnimSKBHeader* data, unsigned int bone)
{
    float quat[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    unsigned int tcount = data->TimeCount;
    unsigned int kcount = data->KeyCount;
    iAnimSKBKey* keys = (iAnimSKBKey*)(data + 1);
    unsigned short* offsets = (unsigned short*)((float*)(keys + kcount) + tcount);
    unsigned int keyfirst = offsets[bone];
    unsigned int keylast = offsets[(tcount - 2) * data->BoneCount + bone] + 1;

    for (unsigned int i = 0; i < kcount; i++)
    {
        if (i >= keyfirst && i <= keylast)
        {
            float _s = quat[3] * keys[i].Quat[3] - quat[0] * keys[i].Quat[0] -
                       quat[1] * keys[i].Quat[1] - quat[2] * keys[i].Quat[2];
            float _x = quat[0] * keys[i].Quat[3] + quat[3] * keys[i].Quat[0] +
                       quat[1] * keys[i].Quat[2] - quat[2] * keys[i].Quat[1];
            float _y = quat[1] * keys[i].Quat[3] + quat[3] * keys[i].Quat[1] +
                       quat[2] * keys[i].Quat[0] - quat[0] * keys[i].Quat[2];
            float _z = quat[2] * keys[i].Quat[3] + quat[3] * keys[i].Quat[2] +
                       quat[0] * keys[i].Quat[1] - quat[1] * keys[i].Quat[0];

            keys[i].Quat[0] = MAX(-32767.0f, MIN(_x, 32767.0f));
            keys[i].Quat[1] = MAX(-32767.0f, MIN(_y, 32767.0f));
            keys[i].Quat[2] = MAX(-32767.0f, MIN(_z, 32767.0f));
            keys[i].Quat[3] = MAX(-32767.0f, MIN(_s, 32767.0f));
            keys[i].Tran[0] = -keys[i].Tran[0];
            keys[i].Tran[2] = -keys[i].Tran[2];
        }
    }
}
