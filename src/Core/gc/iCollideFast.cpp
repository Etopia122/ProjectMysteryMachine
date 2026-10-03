#include "xMath3.h"
#include "xEnt.h"
#include "iEnv.h"
#include "rwtypes.h"

class _xScene;
class RpWorldSector;
class RpCollisionTriangle;
class RpAtomic;

class _xEnv
{
public:
    iEnv* geom;
};

class RwLine
{
public:
    _xVec3 start;
    _xVec3 end;
};

class RpIntersection
{
public:
    RwLine line;     // t.line
    signed int type; // 0x18
};

class xModelInstanceAtomic
{
public:
    unsigned char pad0[0x10];
    RpAtomic* Data; // 0x10
};

extern "C" {
RpWorld* RpCollisionWorldForAllIntersections(
    RpWorld* world, RpIntersection* intersection,
    RpCollisionTriangle* (*callBack)(RpIntersection*, RpWorldSector*, RpCollisionTriangle*, float,
                                     void*),
    void* data);
RpAtomic* RpAtomicForAllIntersections(RpAtomic* atomic, RpIntersection* intersection,
                                      RpCollisionTriangle* (*callBack)(RpIntersection*,
                                                                       RpCollisionTriangle*, float,
                                                                       void*),
                                      void* data);
}

static unsigned int hitzhit;

static RpCollisionTriangle* rayHitsEnvFastCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle*,
                                             float, void*);
static RpCollisionTriangle* rayHitsModelFastCB(RpIntersection*, RpCollisionTriangle*, float, void*);

// Builds a line intersection from a ray in the local `isx`.
#define IRAY_TO_LINE(r, isx)                                                                       \
    isx.type = 1;                                                                                  \
    if ((r)->flags & 0x400)                                                                        \
    {                                                                                              \
        _xVec3 delta;                                                                              \
        xVec3SMul(&delta, &(r)->dir, (r)->min_t);                                                  \
        xVec3Add(&isx.line.start, &(r)->origin, &delta);                                           \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        xVec3Copy(&isx.line.start, &(r)->origin);                                                  \
    }                                                                                              \
    if ((r)->flags & 0x800)                                                                        \
    {                                                                                              \
        float len = ((r)->flags & 0x400) ? (r)->max_t - (r)->min_t : (r)->max_t;                   \
        xVec3SMul(&isx.line.end, &(r)->dir, len);                                                  \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        xVec3Copy(&isx.line.end, &(r)->dir);                                                       \
    }                                                                                              \
    xVec3Add(&isx.line.end, &isx.line.start, &isx.line.end);

#define ILINE_SWAP(isx)                                                                                {                                                                                                      _xVec3 tmp = isx.line.start;                                                                       isx.line.start = isx.line.end;                                                                     isx.line.end = tmp;                                                                            }

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iCollideFastInit(_xScene* sc)
{
}

static RpCollisionTriangle* rayHitsModelFastCB(RpIntersection*, RpCollisionTriangle*, float, void*)
{
    hitzhit = 1;
    return 0;
}

static RpCollisionTriangle* rayHitsEnvFastCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle*,
                                             float, void*)
{
    hitzhit = 1;
    return 0;
}

unsigned int iRayHitsModelFast(const _xRay3* r, const xModelInstance* m)
{
    RpIntersection isx;

    IRAY_TO_LINE(r, isx)
    hitzhit = 0;
    RpAtomicForAllIntersections(((xModelInstanceAtomic*)m)->Data, &isx, rayHitsModelFastCB, 0);
    if (hitzhit)
    {
        return 1;
    }

    ILINE_SWAP(isx)
    RpAtomicForAllIntersections(((xModelInstanceAtomic*)m)->Data, &isx, rayHitsModelFastCB, 0);
    return hitzhit;
}

unsigned int iRayHitsEnvFast(const _xRay3* r, const _xEnv* env)
{
    RpIntersection isx;

    IRAY_TO_LINE(r, isx)
    hitzhit = 0;
    RpCollisionWorldForAllIntersections(env->geom->world, &isx, rayHitsEnvFastCB, 0);
    if (hitzhit)
    {
        return 1;
    }

    ILINE_SWAP(isx)
    RpCollisionWorldForAllIntersections(env->geom->world, &isx, rayHitsEnvFastCB, 0);
    return hitzhit;
}

unsigned int iRayHitsEnvSpecialFast(const _xRay3* r, const _xEnv* env)
{
    RpIntersection isx;

    IRAY_TO_LINE(r, isx)
    hitzhit = 0;
    RpCollisionWorldForAllIntersections(env->geom->world, &isx, rayHitsEnvFastCB, 0);
    if (hitzhit)
    {
        return 1;
    }
    if (env->geom->collision)
    {
        RpCollisionWorldForAllIntersections(env->geom->collision, &isx, rayHitsEnvFastCB, 0);
    }
    if (hitzhit)
    {
        return 1;
    }

    ILINE_SWAP(isx)
    RpCollisionWorldForAllIntersections(env->geom->world, &isx, rayHitsEnvFastCB, 0);
    if (hitzhit)
    {
        return 1;
    }
    if (env->geom->collision)
    {
        RpCollisionWorldForAllIntersections(env->geom->collision, &isx, rayHitsEnvFastCB, 0);
    }
    return hitzhit;
}
