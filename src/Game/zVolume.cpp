#include "xBase.h"
#include "xBound.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xstransvc.h"
#include "xMath.h"

class xSerial;

float xVec3Normalize(_xVec3* o, _xVec3* v);

inline void xVec3Cross(_xVec3* o, const _xVec3* a, const _xVec3* b)
{
    _xVec3 temp;

    temp.x = a->y * b->z - b->y * a->z;
    temp.y = a->z * b->x - b->z * a->x;
    temp.z = a->x * b->y - b->x * a->y;
    xVec3Copy(o, &temp);
}

inline void xVec3Inv(_xVec3* o, const _xVec3* v)
{
    xVec3SMul(o, v, -1.0f);
}

inline float iabs(float x)
{
    return __fabs(x);
}

class zVolumeAsset : public p2BaseAsset
{
public:
    unsigned int flags; // 0x8
    _xBound bound;      // 0xC
    float rot;          // 0x44
    float xpivot;       // 0x48
    float zpivot;       // 0x4C
};

class _zVolume : public xBase
{
public:
    zVolumeAsset* asset; // 0x10
};

class PreCalcOcclude
{
public:
    _xVec4 DepthVec;
    _xVec4 FrustVec[4];
};

static _zVolume* vols;
static unsigned short nvols;
signed int gOccludeCount;
_zVolume* gOccludeList[10];
signed int gOccludeCalcCount;
PreCalcOcclude gOccludeCalc[10];

signed int zVolumeEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam, xBase* toParamWidget);

// Functions are in reverse address order (-inline deferred).

void zVolumeSave(_zVolume* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zVolumeLoad(_zVolume* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

inline void zVolumeInit(_zVolume* vol, zVolumeAsset* asset)
{
    xBaseInit(vol, asset);
    vol->eventFunc = zVolumeEventCB;
    vol->asset = asset;
    if (vol->linkCount)
    {
        vol->link = (p2LinkAsset*)(vol->asset + 1);
    }
    else
    {
        vol->link = 0;
    }
}

void zVolumeInit()
{
    unsigned int size;
    unsigned short i;
    zVolumeAsset* asset;

    nvols = xSTAssetCountByType('VOLU');
    if (nvols)
    {
        vols = (_zVolume*)xMemAlloc(gActiveHeap, nvols * sizeof(_zVolume));
        for (i = 0; i < nvols; i++)
        {
            asset = (zVolumeAsset*)xSTFindAssetByType('VOLU', i, &size);
            zVolumeInit(&vols[i], asset);
        }
    }

    else
    {
        vols = 0;
    }
}

void zVolumeSetup()
{
    unsigned int i;

    for (i = 0; i < nvols; i++)
    {
        vols[i].asset->bound.qcd.xmask = vols[i].asset->bound.qcd.ymask = vols[i].asset->bound.qcd.zmask =
            0xFFFFFFFF;
    }
}

_xBound* zVolumeGetBound(_zVolume* vol)
{
    return &vol->asset->bound;
}

_zVolume* zVolumeGetVolume(unsigned short n)
{
    return &vols[n];
}

void zVolume_OccludePrecalc(_xVec3* camPos)
{
    signed int i;
    signed int j;
    _xVec3 corner[5];
    _zVolume* vol;
    zVolumeAsset* a;
    float c;
    float s;
    PreCalcOcclude* calc;
    _xVec3 d1;
    _xVec3 d2;
    _xVec4 locFrustVec[4];
    float depthdot;
    float camdot;
    float testdot1;
    float testdot2;

    gOccludeCalcCount = 0;

    for (i = 0; i < gOccludeCount; i++)
    {
        vol = gOccludeList[i];
        a = vol->asset;

        c = icos(a->rot);
        s = isin(a->rot);

        calc = &gOccludeCalc[gOccludeCalcCount];

        corner[0].x = c * (a->bound.box.box.lower.x - a->xpivot) + a->xpivot;
        corner[0].y = a->bound.box.box.lower.y;
        corner[0].z = s * (a->bound.box.box.lower.x - a->xpivot) + a->bound.box.box.lower.z;

        corner[1].x = c * (a->bound.box.box.upper.x - a->xpivot) + a->xpivot;
        corner[1].y = a->bound.box.box.lower.y;
        corner[1].z = s * (a->bound.box.box.upper.x - a->xpivot) + a->bound.box.box.lower.z;

        corner[2].x = c * (a->bound.box.box.upper.x - a->xpivot) + a->xpivot;
        corner[2].y = a->bound.box.box.upper.y;
        corner[2].z = s * (a->bound.box.box.upper.x - a->xpivot) + a->bound.box.box.lower.z;

        corner[3].x = c * (a->bound.box.box.lower.x - a->xpivot) + a->xpivot;
        corner[3].y = a->bound.box.box.upper.y;
        corner[3].z = s * (a->bound.box.box.lower.x - a->xpivot) + a->bound.box.box.lower.z;

        corner[4] = corner[0];

        xVec3Sub(&d1, &corner[1], &corner[0]);
        xVec3Sub(&d2, &corner[2], &corner[0]);

        xVec3Cross((_xVec3*)calc, &d1, &d2);
        xVec3Normalize((_xVec3*)calc, (_xVec3*)calc);

        depthdot = xVec3Dot((_xVec3*)calc, &corner[0]);
        camdot = xVec3Dot((_xVec3*)calc, camPos);

        if (camdot > depthdot)
        {
            xVec3Inv((_xVec3*)calc, (_xVec3*)calc);

            camdot = -camdot;
            depthdot = -depthdot;
        }

        if (!(iabs(camdot - depthdot) < 1.0f))

        {
            calc->DepthVec.w = depthdot;

            for (j = 0; j < 4; j++)
            {
                xVec3Sub(&d1, &corner[j], camPos);
                xVec3Sub(&d2, &corner[j + 1], camPos);
                xVec3Cross((_xVec3*)&locFrustVec[j], &d1, &d2);
                xVec3Normalize((_xVec3*)&locFrustVec[j], (_xVec3*)&locFrustVec[j]);
            }

            testdot1 = xVec3Dot((_xVec3*)&locFrustVec[0], &corner[0]);
            testdot2 = xVec3Dot((_xVec3*)&locFrustVec[0], &corner[2]);

            if (testdot1 > testdot2)
            {
                for (j = 0; j < 4; j++)
                {
                    xVec3Inv((_xVec3*)&locFrustVec[j], (_xVec3*)&locFrustVec[j]);
                }
            }

            for (j = 0; j < 4; j++)
            {
                locFrustVec[j].w = xVec3Dot((_xVec3*)&locFrustVec[j], &corner[j]);
            }

            calc->FrustVec[0].x = locFrustVec[0].w;
            calc->FrustVec[0].y = locFrustVec[1].w;
            calc->FrustVec[0].z = locFrustVec[2].w;
            calc->FrustVec[0].w = locFrustVec[3].w;

            calc->FrustVec[1].x = locFrustVec[0].x;
            calc->FrustVec[1].y = locFrustVec[1].x;
            calc->FrustVec[1].z = locFrustVec[2].x;
            calc->FrustVec[1].w = locFrustVec[3].x;

            calc->FrustVec[2].x = locFrustVec[0].y;
            calc->FrustVec[2].y = locFrustVec[1].y;
            calc->FrustVec[2].z = locFrustVec[2].y;
            calc->FrustVec[2].w = locFrustVec[3].y;

            calc->FrustVec[3].x = locFrustVec[0].z;
            calc->FrustVec[3].y = locFrustVec[1].z;
            calc->FrustVec[3].z = locFrustVec[2].z;
            calc->FrustVec[3].w = locFrustVec[3].z;

            gOccludeCalcCount++;
        }
    }
}

signed int zVolumeEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam, xBase* toParamWidget)
{
    _zVolume* vol = (_zVolume*)to;
    signed int i;

    switch (toEvent)
    {
    case 10:
        xBaseReset(vol, vol->asset);
        break;
    case 0x108:
        if (gOccludeCount == 10)
        {
            return 1;
        }
        for (i = 0; i < gOccludeCount; i++)
        {
            if (gOccludeList[i] == vol)
            {
                return 1;
            }
        }
        gOccludeList[gOccludeCount] = vol;
        gOccludeCount++;
        break;
    case 0x109:
        for (i = 0; i < gOccludeCount; i++)
        {
            if (gOccludeList[i] == vol)
            {
                gOccludeList[gOccludeCount] = gOccludeList[gOccludeCount - 1];
                gOccludeCount--;
                return 1;
            }
        }
        break;
    }

    return 1;
}
