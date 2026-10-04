#include "xMath.h"
#include "xMath3.h"
#include "xCollide.h"
#include "xEnt.h"
#include "iEnv.h"
#include "rwtypes.h"

extern "C" void* memcpy(void* dst, const void* src, unsigned int n);

class _xScene;
class RpAtomic;

class _xEnv
{
public:
    iEnv* geom;
};

class RwSphere
{
public:
    _xVec3 center;
    float radius;
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
    union
    {
        RwLine line;
        RwSphere sphere;
    };
    signed int type; // 0x18
};

class RpCollisionTriangle
{
public:
    RwV3d normal;      // 0x0
    RwV3d point;       // 0xC
    signed int index;  // 0x18
    RwV3d* vertices[3]; // 0x1C
};

class RpPolygon
{
public:
    unsigned short matIndex;
    unsigned short vertIndex[3];
};

class RpWorldSector
{
public:
    signed int type;
    RpPolygon* polygons; // 0x4
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
RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrixTag* m, signed int combine);
}

unsigned char xCollideGetCollsIdx(_xVec3* tohit, _xMat3x3* mat);
void xMat3x3Normalize(_xMat3x3* o, _xMat3x3* m);

static float cbath;
static _xRay3 cbray;
static const _xMat3x3* cbmat;
static RpIntersection cbisx_local;
static unsigned char cbnumcs;
static unsigned char cbmaxcs;
static unsigned char FLOOR;
static unsigned char NEXT2;
static unsigned char OTHER;

enum Dimension
{
    XDIM = 0,
    YDIM = 1,
    ZDIM = 2,
};

#define FLOAT_MAX 1e19f

// Builds a line intersection from a ray.
static inline void iRayToLine(RpIntersection* isx, const _xRay3* r)
{
    isx->type = 1;
    if (r->flags & 0x400)
    {
        _xVec3 delta;
        xVec3SMul(&delta, &r->dir, r->min_t);
        xVec3Add(&isx->line.start, &r->origin, &delta);
    }
    else
    {
        xVec3Copy(&isx->line.start, &r->origin);
    }
    if (r->flags & 0x800)
    {
        float len = (r->flags & 0x400) ? r->max_t - r->min_t : r->max_t;
        xVec3SMul(&isx->line.end, &r->dir, len);
    }
    else
    {
        xVec3Copy(&isx->line.end, &r->dir);
    }
    xVec3Add(&isx->line.end, &isx->line.start, &isx->line.end);
}

#define ILINE_SWAP(isx)                                                                            \
    {                                                                                              \
        _xVec3 tmp = isx.line.start;                                                               \
        isx.line.start = isx.line.end;                                                             \
        isx.line.end = tmp;                                                                        \
    }

static inline float iabs(float x)
{
    return __fabs(x);
}

static inline float xVec3Length2(const _xVec3* v)
{
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

#pragma fp_contract off
static inline float xVec3LengthNC(const _xVec3* v)
{
    return xsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}
#pragma fp_contract on

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iCollideInit(_xScene*)
{
}

// TODO: 99.8%. absY/absZ land in f2/f3 instead of f1/f2.
static signed int PointWithinTriangle(RwV3d* pt, RwV3d** tri, RwV3d* normal)
{
    Dimension dimension;
    float absX;
    float absY;
    float absZ;
    signed int inside;
    signed int i;
    signed int j;

    absX = iabs(normal->x);
    absY = iabs(normal->y);
    absZ = iabs(normal->z);
    inside = 0;

    if (absZ > absY)
    {
        if (absZ > absX)
        {
            dimension = ZDIM;
        }
        else
        {
            dimension = XDIM;
        }
    }
    else
    {
        if (absY > absX)
        {
            dimension = YDIM;
        }
        else
        {
            dimension = XDIM;
        }
    }

    switch (dimension)
    {
    case XDIM:
        for (i = 0, j = 2; i < 3; j = i, i++)
        {
            if (((tri[i]->y <= pt->y && pt->y < tri[j]->y) ||
                 (tri[j]->y <= pt->y && pt->y < tri[i]->y)) &&
                (pt->z < tri[i]->z + ((pt->y - tri[i]->y) * (tri[j]->z - tri[i]->z)) /
                                         (tri[j]->y - tri[i]->y)))
            {
                inside = !inside;
            }
        }
        break;
    case YDIM:
        for (i = 0, j = 2; i < 3; j = i, i++)
        {
            if (((tri[i]->z <= pt->z && pt->z < tri[j]->z) ||
                 (tri[j]->z <= pt->z && pt->z < tri[i]->z)) &&
                (pt->x < tri[i]->x + ((pt->z - tri[i]->z) * (tri[j]->x - tri[i]->x)) /
                                         (tri[j]->z - tri[i]->z)))
            {
                inside = !inside;
            }
        }
        break;
    case ZDIM:
        for (i = 0, j = 2; i < 3; j = i, i++)
        {
            if (((tri[i]->y <= pt->y && pt->y < tri[j]->y) ||
                 (tri[j]->y <= pt->y && pt->y < tri[i]->y)) &&
                (pt->x < tri[i]->x + ((pt->y - tri[i]->y) * (tri[j]->x - tri[i]->x)) /
                                         (tri[j]->y - tri[i]->y)))
            {
                inside = !inside;
            }
        }
        break;
    }

    return inside;
}

static inline void FindNearestPointOnLine(RwV3d* result, RwV3d* point, RwV3d* start, RwV3d* end)
{
    RwV3d localResult;
    float mu;
    float lineLength2;

    mu = (point->x * (end->x - start->x) + point->y * (end->y - start->y) +
          point->z * (end->z - start->z)) -
         (start->x * (end->x - start->x) + start->y * (end->y - start->y) +
          start->z * (end->z - start->z));
    if (mu <= 0.0f)
    {
        localResult = *start;
    }
    else
    {
        lineLength2 = (end->x - start->x) * (end->x - start->x) +
                      (end->y - start->y) * (end->y - start->y) +
                      (end->z - start->z) * (end->z - start->z);
        if (mu < lineLength2)
        {
            mu /= lineLength2;
            localResult.x = mu * (end->x - start->x);
            localResult.x += start->x;
            localResult.y = mu * (end->y - start->y);
            localResult.y += start->y;
            localResult.z = mu * (end->z - start->z);
            localResult.z += start->z;
        }
        else
        {
            localResult = *end;
        }
    }
    *result = localResult;
}

// TODO: 99.0%. The inlined FindNearestPointOnLine keeps the start vertex in
// f13/f29/f30 and the edge deltas in f9/f10/f12; ours swaps them.
static void properSphereIsectTri(const _xVec3* center, float radius, _xVec3* tohit,
                                 float* dist_ptr, RpCollisionTriangle* tri)
{
    _xVec3 projPoint;
    float dist2plane;
    float dist;
    float dist2;
    float radius2;
    unsigned int i;
    _xVec3 vertClosestPoint;
    _xVec3 temp;

    dist = *dist_ptr;
    dist2plane = xVec3Dot((_xVec3*)&tri->normal, (_xVec3*)&tri->point) -
                 xVec3Dot((_xVec3*)&tri->normal, center);

    xVec3SMul(&projPoint, (_xVec3*)&tri->normal, dist2plane);
    xVec3Copy(tohit, &projPoint);
    xVec3AddTo(&projPoint, center);

    if (PointWithinTriangle((RwV3d*)&projPoint, tri->vertices, &tri->normal))
    {
        dist = iabs(dist2plane);
    }
    else
    {
        dist2 = dist * dist;
        radius2 = radius * radius;
        for (i = 0; i < 3; i++)
        {
            FindNearestPointOnLine((RwV3d*)&vertClosestPoint, (RwV3d*)&projPoint,
                                   tri->vertices[i], tri->vertices[(i + 1) % 3]);
            xVec3Sub(&temp, &vertClosestPoint, center);
            float vertDist2 = xVec3Length2(&temp);

            if (vertDist2 < dist2 && vertDist2 < radius2)
            {
                dist2 = vertDist2;
                xVec3Copy(tohit, &temp);
            }
        }
        dist = xsqrt(dist2);
    }
    *dist_ptr = dist;
}

// TODO: 99.6%. The original stores OTHER before NEXT2 in the NEXT2 == 0xff
// branch and truncates idx again (clrlwi) after storing it.
static RpCollisionTriangle* sphereHitsEnv3CB(RpIntersection* isx, RpWorldSector* sector,
                                             RpCollisionTriangle* tri, float dist, void* data)
{
    _xCollis* colls = (_xCollis*)data;
    float radius = isx->sphere.radius;
    _xVec3 tohit;
    _xVec3 hdng;
    unsigned char idx;
    float scale;

    dist = FLOAT_MAX;
    properSphereIsectTri(&isx->sphere.center, radius, &tohit, &dist, tri);
    if (0.0f == dist)
    {
        return tri;
    }
    if (dist > radius)
    {
        return tri;
    }

    if (tohit.y * tohit.y > tohit.x * tohit.x + tohit.z * tohit.z)
    {
        if (FLOOR == 0xff)
        {
            FLOOR = cbnumcs;
            idx = cbnumcs;
            cbnumcs++;
        }
        else if (tohit.y < 0.0f)
        {
            if (colls[FLOOR].hdng.y > 0.0f || dist < colls[FLOOR].dist ||
                (iabs(dist - colls[FLOOR].dist) < 0.001f && tri->normal.y > colls[FLOOR].norm.y))
            {
                idx = FLOOR;
            }
            else
            {
                return tri;
            }
        }
        else
        {
            if (colls[FLOOR].hdng.y > 0.0f &&
                (dist < colls[FLOOR].dist ||
                 (iabs(dist - colls[FLOOR].dist) < 0.001f && tri->normal.y > colls[FLOOR].norm.y)))
            {
                idx = FLOOR;
            }
            else
            {
                return tri;
            }
        }

        xVec3SMul(&hdng, &tohit, 1.0f / dist);
    }
    else if (NEXT2 == 0xff)
    {
        if (OTHER == 0xff)
        {
            xVec3SMul(&hdng, &tohit, 1.0f / dist);
            OTHER = cbnumcs;
            idx = cbnumcs;
            cbnumcs++;
        }
        else
        {
            xVec3SMul(&hdng, &tohit, 1.0f / dist);
            if (iabs(xVec3Dot(&hdng, &colls[OTHER].hdng)) > 0.70710677f)
            {
                if (dist < colls[OTHER].dist)
                {
                    idx = OTHER;
                }
                else
                {
                    return tri;
                }
            }
            else
            {
                idx = cbnumcs;
                NEXT2 = idx;
                cbnumcs = idx + 1;
                if (dist < colls[OTHER].dist)
                {
                    NEXT2 = OTHER;
                    OTHER = idx;
                }
            }
        }
    }
    else
    {
        if (dist > colls[NEXT2].dist)
        {
            return tri;
        }
        xVec3SMul(&hdng, &tohit, 1.0f / dist);
        float odot = xVec3Dot(&hdng, &colls[OTHER].hdng);
        if (dist < colls[OTHER].dist)
        {
            if (iabs(odot) > 0.70710677f)
            {
                if (iabs(xVec3Dot(&hdng, &colls[NEXT2].hdng)) > 0.70710677f)
                {
                    if (NEXT2 < OTHER)
                    {
                        OTHER = NEXT2;
                    }
                    NEXT2 = 0xff;
                    cbnumcs--;
                    idx = OTHER;
                    colls[cbnumcs].flags &= ~1;
                    colls[cbnumcs].dist = FLOAT_MAX;
                }
                else
                {
                    idx = OTHER;
                }
            }
            else
            {
                idx = NEXT2;
            }
        }
        else
        {
            if (iabs(odot) > 0.70710677f)
            {
                return tri;
            }
            idx = NEXT2;
        }
    }

    colls[idx].dist = dist;
    xVec3Copy(&colls[idx].hdng, &hdng);

    if (colls->flags & 0x200)
    {
        xVec3Copy(&colls[idx].norm, (_xVec3*)&tri->normal);
    }
    if (colls->flags & 0x400)
    {
        xVec3Copy(&colls[idx].tohit, &tohit);
    }
    if (colls->flags & 0x800)
    {
        scale = dist - radius;
        xVec3Copy(&colls[idx].depen, &hdng);
        xVec3SMulBy(&colls[idx].depen, scale);
    }
    colls[idx].flags |= 1;

    if (sector != 0)
    {
        colls[idx].oid = sector->polygons[tri->index].matIndex;
    }
    else
    {
        colls[idx].oid = tri->index;
    }

    if (cbnumcs == cbmaxcs)
    {
        return 0;
    }

    return tri;
}

static RpCollisionTriangle* sphereHitsEnv4CB(RpIntersection* isx, RpWorldSector* sector,
                                             RpCollisionTriangle* tri, float dist, void* data)
{
    _xCollis* colls = (_xCollis*)data;
    float radius = isx->sphere.radius;
    _xVec3 tohit;
    _xCollis* c;
    float ddist;

    dist = FLOAT_MAX;
    properSphereIsectTri(&isx->sphere.center, radius, &tohit, &dist, tri);
    if (0.0f == dist)
    {
        return tri;
    }
    if (dist > radius)
    {
        return tri;
    }

    c = &colls[xCollideGetCollsIdx(&tohit, (_xMat3x3*)cbmat)];
    ddist = dist - c->dist;

    if (!(ddist < 0.0f || (ddist < 0.001f && c->norm.y > tri->normal.y)))
    {
        return tri;
    }

    c->dist = dist;
    c->tohit = tohit;
    c->norm.x = tri->normal.x;
    c->norm.y = tri->normal.y;
    c->norm.z = tri->normal.z;
    c->flags |= 1;

    if (sector != 0)
    {
        c->oid = sector->polygons[tri->index].matIndex;
    }
    else
    {
        c->oid = tri->index;
    }

    return tri;
}

static RpCollisionTriangle* sphereHitsModel3CB(RpIntersection* isx, RpCollisionTriangle* tri,
                                               float dist, void* data)
{
    return sphereHitsEnv3CB(&cbisx_local, 0, tri, dist, data);
}

#pragma auto_inline on
static RpCollisionTriangle* rayHitsEnvCB(RpIntersection* isx, RpWorldSector* sector,
                                         RpCollisionTriangle* tri, float dist, void* data)
{
    _xCollis* coll = (_xCollis*)data;

    dist *= cbray.max_t;
    if (dist >= coll->dist)
    {
        return tri;
    }

    if (sector != 0)
    {
        coll->oid = sector->polygons[tri->index].matIndex;
    }
    else
    {
        coll->oid = tri->index;
    }

    coll->dist = dist;
    coll->flags |= 1;
    if (coll->flags & 0x200)
    {
        xVec3Copy(&coll->norm, (_xVec3*)&tri->normal);
    }

    return tri;
}

static RpCollisionTriangle* rayHitsEnvBackwardCB(RpIntersection* isx, RpWorldSector* sector,
                                                 RpCollisionTriangle* tri, float dist, void* data)
{
    _xCollis* coll = (_xCollis*)data;

    dist = 1.0f - dist;
    dist = dist * cbray.max_t;
    if (dist >= coll->dist)
    {
        return tri;
    }

    if (sector != 0)
    {
        coll->oid = sector->polygons[tri->index].matIndex;
    }
    else
    {
        coll->oid = tri->index;
    }

    coll->dist = dist;
    coll->flags |= 1;

    return tri;
}

#pragma auto_inline reset

static RpCollisionTriangle* rayHitsModelCB(RpIntersection* isx, RpCollisionTriangle* tri,
                                           float dist, void* data)
{
    return rayHitsEnvCB(isx, 0, tri, dist, data);
}

static RpCollisionTriangle* rayHitsModelBackwardCB(RpIntersection* isx, RpCollisionTriangle* tri,
                                                   float dist, void* data)
{
    return rayHitsEnvBackwardCB(isx, 0, tri, dist, data);
}

// TODO: 99.8%. cbnumcs = 0 uses r3 instead of r4.
signed int iSphereHitsEnv3(const _xSphere* b, const _xEnv* env, _xCollis* colls,
                           unsigned char ncolls, float sth)
{
    RpIntersection isx;
    unsigned char idx;

    isx.type = 3;
    memcpy(&isx.sphere, b, sizeof(_xSphere));

    for (idx = 0; idx < ncolls; idx++)
    {
        colls[idx].flags &= ~1;
        colls[idx].dist = FLOAT_MAX;
    }

    cbnumcs = 0;
    cbath = sth;
    cbmaxcs = ncolls;
    OTHER = 0xff;
    NEXT2 = 0xff;
    FLOOR = 0xff;

    RpCollisionWorldForAllIntersections(env->geom->world, &isx, sphereHitsEnv3CB, colls);
    if (env->geom->collision != 0)
    {
        RpCollisionWorldForAllIntersections(env->geom->collision, &isx, sphereHitsEnv3CB, colls);
    }

    return cbnumcs;
}

signed int iSphereHitsEnv4(const _xSphere* b, const _xEnv* env, const _xMat3x3* mat,
                           _xCollis* colls)
{
    RpIntersection isx;
    _xCollis* c;
    _xCollis* cend;

    isx.type = 3;
    memcpy(&isx.sphere, b, sizeof(_xSphere));

    cbmat = mat;
    cend = colls + 6;

    RpCollisionWorldForAllIntersections(env->geom->world, &isx, sphereHitsEnv4CB, colls);

    for (c = colls; c < cend; c++)
    {
        if (c->flags & 1)
        {
            c->oid += 0x10000;
        }
    }

    if (env->geom->collision != 0)
    {
        RpCollisionWorldForAllIntersections(env->geom->collision, &isx, sphereHitsEnv4CB, colls);

        for (c = colls; c < cend; c++)
        {
            if (c->flags & 1)
            {
                if (c->oid < 0x10000)
                {
                    c->oid += 0x10000;
                }
                else
                {
                    c->oid -= 0x10000;
                }
            }
        }
    }

    signed int numcs = 0;
    for (c = colls; c < cend; c++)
    {
        if (c->flags & 1)
        {
            numcs++;
            float s = 1.0f / c->dist;
            c->hdng.x = c->tohit.x * s;
            c->hdng.y = c->tohit.y * s;
            c->hdng.z = c->tohit.z * s;

            s = (0.0f < c->dist - b->r) ? 0.0f : c->dist - b->r;
            c->depen.x = c->hdng.x * s;
            c->depen.y = c->hdng.y * s;
            c->depen.z = c->hdng.z * s;

            if (xVec3Dot(&c->hdng, &c->norm) > 0.0f)
            {
                c->norm.x = -c->norm.x;
                c->norm.y = -c->norm.y;
                c->norm.z = -c->norm.z;
            }
        }
    }

    return numcs;
}

signed int iSphereHitsModel3(const _xSphere* b, const xModelInstance* m, _xCollis* colls,
                             unsigned char ncolls, float sth)
{
    _xMat3x3 mnormal;
    _xMat4x3* mat;
    unsigned char idx;
    RpIntersection isx;

    isx.type = 3;
    memcpy(&isx.sphere, b, sizeof(_xSphere));

    mat = (_xMat4x3*)m->Mat;
    RwFrameTransform((RwFrame*)((RwObject*)m->Data)->parent, (RwMatrixTag*)mat, 0);
    float mscale = xVec3LengthNC(&mat->right);
    xMat3x3Normalize(&mnormal, (_xMat3x3*)mat);
    xMat4x3Tolocal(&cbisx_local.sphere.center, mat, &b->center);

    cbisx_local.sphere.radius = b->r / mscale;

    for (idx = 0; idx < ncolls; idx++)
    {
        colls[idx].flags &= ~1;
        colls[idx].dist = FLOAT_MAX;
    }

    cbnumcs = 0;
    cbath = sth;
    cbmaxcs = ncolls;
    OTHER = 0xff;
    NEXT2 = 0xff;
    FLOOR = 0xff;

    RpAtomicForAllIntersections(m->Data, &isx, sphereHitsModel3CB, colls);

    if (colls->flags & 0x1F00)
    {
        for (idx = 0; idx < cbnumcs; idx++)
        {
            if (colls[idx].flags & 1)
            {
                xMat3x3RMulVec(&colls[idx].tohit, (_xMat3x3*)mat, &colls[idx].tohit);
                xMat3x3RMulVec(&colls[idx].depen, (_xMat3x3*)mat, &colls[idx].depen);
                xMat3x3RMulVec(&colls[idx].hdng, &mnormal, &colls[idx].hdng);
                xMat3x3RMulVec(&colls[idx].norm, &mnormal, &colls[idx].norm);
                colls[idx].dist *= mscale;
            }
        }
    }

    return cbnumcs;
}

unsigned int iRayHitsEnv(const _xRay3* r, const _xEnv* env, _xCollis* coll)
{
    RpIntersection isx;

    iRayToLine(&isx, r);
    coll->flags &= ~1;
    coll->dist = FLOAT_MAX;
    cbray = *r;

    if (!(r->flags & 0x800))
    {
        cbray.max_t = 1.0f;
    }
    if (r->flags & 0x400)
    {
        cbray.max_t -= cbray.min_t;
    }

    RpCollisionWorldForAllIntersections(env->geom->world, &isx, rayHitsEnvCB, coll);
    if (env->geom->collision != 0)
    {
        RpCollisionWorldForAllIntersections(env->geom->collision, &isx, rayHitsEnvCB, coll);
    }

    ILINE_SWAP(isx)

    RpCollisionWorldForAllIntersections(env->geom->world, &isx, rayHitsEnvBackwardCB, coll);
    if (env->geom->collision != 0)
    {
        RpCollisionWorldForAllIntersections(env->geom->collision, &isx, rayHitsEnvBackwardCB,
                                            coll);
    }

    if (r->flags & 0x400)
    {
        coll->dist += cbray.min_t;
    }

    return coll->flags & 1;
}

unsigned int iRayHitsCamEnv(const _xRay3* r, const _xEnv* env, _xCollis* coll)
{
    RpIntersection isx;

    iRayToLine(&isx, r);
    coll->flags &= ~1;
    coll->dist = FLOAT_MAX;
    cbray = *r;

    if (!(r->flags & 0x800))
    {
        cbray.max_t = 1.0f;
    }
    if (r->flags & 0x400)
    {
        cbray.max_t -= cbray.min_t;
    }

    if (env->geom->camera != 0)
    {
        RpCollisionWorldForAllIntersections(env->geom->camera, &isx, rayHitsEnvCB, coll);
    }
    else
    {
        RpCollisionWorldForAllIntersections(env->geom->world, &isx, rayHitsEnvCB, coll);
    }

    ILINE_SWAP(isx)

    if (env->geom->camera != 0)
    {
        RpCollisionWorldForAllIntersections(env->geom->camera, &isx, rayHitsEnvBackwardCB, coll);
    }
    else
    {
        RpCollisionWorldForAllIntersections(env->geom->world, &isx, rayHitsEnvBackwardCB, coll);
    }

    if (r->flags & 0x400)
    {
        coll->dist += cbray.min_t;
    }

    return coll->flags & 1;
}

unsigned int iRayHitsModel(const _xRay3* r, const xModelInstance* m, _xCollis* coll)
{
    _xMat4x3* mat;
    float len;
    RpIntersection isx;

    iRayToLine(&isx, r);
    cbray = *r;

    if (!(r->flags & 0x800))
    {
        cbray.max_t = 1.0f;
    }
    if (r->flags & 0x400)
    {
        cbray.max_t -= cbray.min_t;
    }

    mat = (_xMat4x3*)m->Mat;
    RwFrameTransform((RwFrame*)((RwObject*)m->Data)->parent, (RwMatrixTag*)mat, 0);
    len = xVec3LengthNC(&mat->right);
    cbray.max_t /= len;
    coll->flags &= ~1;
    coll->dist = FLOAT_MAX;

    RpAtomicForAllIntersections(m->Data, &isx, rayHitsModelCB, coll);

    ILINE_SWAP(isx)

    RpAtomicForAllIntersections(m->Data, &isx, rayHitsModelBackwardCB, coll);

    if (r->flags & 0x400)
    {
        coll->dist += cbray.min_t;
    }

    return coll->flags & 1;
}
