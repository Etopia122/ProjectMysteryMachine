#include "xEnt.h"
#include "xMemMgr.h"
#include "xString.h"
#include "xMath.h"
#include "iTime.h"

extern "C" {
void* memcpy(void* dst, const void* src, unsigned int n);
double fmod(double x, double y);
double sin(double x);
double cos(double x);
}

class RxPipeline;

class RpMorphTarget
{
public:
    void* parentGeom;        // 0x0
    float boundingSphere[4]; // 0x4
    _xVec3* verts;           // 0x14
};

class RpGeometry
{
public:
    unsigned char pad0[0x14];
    signed int numVertices; // 0x14
    unsigned char pad18[0x68 - 0x18];
    RpMorphTarget* morphTarget; // 0x68
};

class RpAtomic
{
public:
    unsigned char pad0[0x18];
    RpGeometry* geometry; // 0x18
};

class xModelInstanceData
{
public:
    unsigned char pad0[0x10];
    RpAtomic* Data; // 0x10
};

extern "C" {
RpGeometry* RpGeometryLock(RpGeometry* geometry, signed int lockMode);
RpGeometry* RpGeometryUnlock(RpGeometry* geometry);
}

_xEnt* warbEnt;
static _xVec3* vbuf;
static _xVec3* nbuf;
static unsigned int warbMode;
static float warbTime;
static float warbc[4] = { 0.5f, 2.0f, 0.9f, 1.0f };

static void iFXwarb(_xVec3* orig_verts, _xVec3* norms, RpGeometry* geom);

#define WARB_2PI (2.0f * 3.1415927f)

// Functions are in reverse DOL order (-inline deferred emits last to first).

RxPipeline* iFXanimUVCreatePipe()
{
    return 0;
}

void iFXWarbSetParams(unsigned int mode, float* val);

void iFXWarbInit(void* vp_ent, unsigned int warb_mode)
{
    _xEnt* ent = (_xEnt*)vp_ent;

    if (!ent)
    {
        return;
    }

    RpGeometry* geom = ((xModelInstanceData*)ent->model)->Data->geometry;
    signed int n = geom->numVertices;
    _xVec3* verts = geom->morphTarget->verts;
    vbuf = (_xVec3*)xMemAlloc(gActiveHeap, n * sizeof(_xVec3));
    memcpy(vbuf, verts, n * 12);
    if (ent->id == xStrHash("Tide"))
    {
        ent->material_alpha = 230.0f;
        ent->asset->seeThruSpeed = 0.0f;
    }
    float coeffs[4] = { 0.5f, 2.0f, 0.9f, 1.0f };
    iFXWarbSetParams(warb_mode, (float*)&coeffs);
    warbEnt = ent;
    warbTime = 0.0f;
}

#pragma auto_inline on
void iFXWarbSetParams(unsigned int mode, float* val)
{
    warbMode = mode;
    if (mode == 0)
    {
        warbEnt = 0;
        vbuf = 0;
        nbuf = 0;
    }
    if (val)
    {
        memcpy(warbc, val, sizeof(warbc));
    }
}
#pragma auto_inline off

void iFXWarbUpdate(float dt)
{
    warbTime += dt;
    if (warbEnt)
    {
        iTimeGet();
        iFXwarb(vbuf, nbuf, ((xModelInstanceData*)warbEnt->model)->Data->geometry);
        iTimeGet();
    }
}

// TODO (98.9%): mode 5 copies the first y into both bounds with two fmr, and
// mode 6 keeps an empty branch for the min.
static void iFXwarb(_xVec3* orig_verts, _xVec3* norms, RpGeometry* geom)
{
    _xVec3* ov;
    _xVec3* nv;
    signed int i;
    float t;
    float miny;
    float maxy;

    if (geom && RpGeometryLock(geom, 6))
    {
    ov = orig_verts;
    t = warbTime;
    nv = geom->morphTarget->verts;

    switch (warbMode)
    {
    case 1:
        for (i = 0; i < geom->numVertices; i++, ov++, nv++)
        {
            float a = fmod(warbc[1] * (ov->x + t), WARB_2PI);
            float b = fmod(warbc[3] * (ov->z + t), WARB_2PI);
            nv->y = ov->y + warbc[0] * isin(a) + warbc[2] * isin(b);
        }
        break;
    case 2:
        for (i = 0; i < geom->numVertices; i++, ov++, nv++)
        {
            float x = xsqrt(ov->x * ov->x + ov->y * ov->y + ov->z * ov->z);
            float a = fmod(warbc[1] * (x + t), WARB_2PI);
            nv->y = warbc[0] * icos(a) + ov->y;
        }
        break;
    case 3:
        for (i = 0; i < geom->numVertices; i++, ov++, nv++)
        {
            float a = fmod(warbc[1] * (ov->x + t), WARB_2PI);
            nv->y = warbc[0] * isin(a) + ov->y;
        }
        break;
    case 4:
        for (i = 0; i < geom->numVertices; i++, ov++, nv++)
        {
            float a = fmod(warbc[1] * (ov->x + t), WARB_2PI);
            nv->y = warbc[0] * isin(a) + ov->y;
        }
        break;
    case 5:
    {
        float ooy;
        miny = maxy = orig_verts->y;
        for (i = 0; i < geom->numVertices; i++, ov++)
        {
            miny = miny < ov->y ? miny : ov->y;
            maxy = maxy > ov->y ? maxy : ov->y;
        }
        ov = orig_verts;
        miny = 0.5f * (maxy - miny) + miny;
        ooy = 1.0f / (maxy - miny);
        for (i = 0; i < geom->numVertices; i++, ov++, nv++)
        {
            float mul;
            if (ov->y >= miny)
            {
                float percent = ooy * (ov->y - miny);
                float x = warbc[1] * (-t + 0.4 * percent);
                float a = fmod(x, WARB_2PI);
                mul = 0.5f * (percent * (warbc[0] * isin(a)));
            }
            else
            {
                mul = 0.0f;
            }
            nv->x = ov->x + mul;
            nv->z = ov->z + mul;
        }
        break;
    }
    case 6:
    {
        float edge;
        miny = maxy = orig_verts->y;
        for (i = 0; i < geom->numVertices; i++, ov++)
        {
            miny = miny < ov->y ? miny : ov->y;
            maxy = maxy > ov->y ? maxy : ov->y;
        }
        ov = orig_verts;
        edge = 0.1f * (maxy - miny);
        miny += edge;
        maxy -= edge;
        for (i = 0; i < geom->numVertices; i++, ov++, nv++)
        {
            float mul;
            if (ov->y >= miny && ov->y <= maxy)
            {
                mul = ov->y - miny;
                mul = (mul < maxy - ov->y) ? mul : maxy - ov->y;
                float a = fmod(warbc[1] * (-t + mul), WARB_2PI);
                mul = mul * (warbc[0] * isin(a));
            }
            else
            {
                mul = 0.0f;
            }
            nv->x = ov->x + mul;
            nv->z = ov->z + mul;
        }
        break;
    }
    }

    RpGeometryUnlock(geom);
    }
}
