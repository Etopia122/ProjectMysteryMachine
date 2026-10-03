#include "xBound.h"
#include "xEnt.h"
#include "xScene.h"
#include "xMath.h"

void xMat3x3Normalize(_xMat3x3* o, _xMat3x3* m);
void iBoxIsectRay(_xBox* b, _xRay3* r, _xIsect* isx);
void iCylinderIsectRay(_xCylinder* c, _xRay3* r, _xIsect* isx);
void iCollideFastInit(_xScene* sc);
unsigned int iRayHitsEnvFast(const _xRay3* r, const _xEnv* env);
unsigned int iRayHitsEnvSpecialFast(const _xRay3* r, const _xEnv* env);
unsigned int iRayHitsModelFast(const _xRay3* r, const xModelInstance* m);
void xQuickCullForRay(xQCData* q, _xRay3* r);

static inline unsigned int xRayHitsSphereFast(const _xRay3* r, const _xSphere* s)
{
    _xVec3 length;
    xVec3Sub(&length, &r->origin, &s->center);
    float d = xVec3Dot(&length, &length) - s->r * s->r;
    if (d <= 0.0f)
    {
        return 1;
    }
    if ((r->flags & 0x800) && d > r->max_t * (2.0f * s->r + r->max_t))
    {
        return 0;
    }
    float dot = xVec3Dot(&length, &r->dir);
    if (dot >= 0.0f)
    {
        return 0;
    }
    return dot * dot >= d;
}

static inline unsigned char xRayHitsCylinderFast(const _xRay3* r, const _xCylinder* c)
{
    _xIsect isect;
    iCylinderIsectRay((_xCylinder*)c, (_xRay3*)r, &isect);
    unsigned char hit = 0;
    if (isect.penned <= 0.0f || isect.contained <= 0.0f)
    {
        hit = 1;
    }
    return hit;
}

static inline unsigned char xRayHitsBoxFast(const _xRay3* r, const _xBox* b)
{
    _xIsect isect;
    iBoxIsectRay((_xBox*)b, (_xRay3*)r, &isect);
    unsigned char hit = 0;
    if (isect.penned <= 0.0f || isect.contained <= 0.0f)
    {
        hit = 1;
    }
    return hit;
}

static inline unsigned char xRayHitsOBBFast(const _xRay3* r, const _xBound* b)
{
    _xIsect isect;
    _xMat3x3 mn;
    _xRay3 lr;
    float scale;

    scale = xsqrt(b->mat->right.x * b->mat->right.x + b->mat->right.y * b->mat->right.y +
                  b->mat->right.z * b->mat->right.z);
    xMat3x3Normalize(&mn, (_xMat3x3*)b->mat);
    xMat4x3Tolocal(&lr.origin, b->mat, &r->origin);
    xMat3x3Tolocal(&lr.dir, &mn, (_xVec3*)&r->dir);
    lr.flags = r->flags;
    lr.max_t = r->max_t / scale;
    lr.min_t = r->min_t / scale;
    iBoxIsectRay((_xBox*)&b->box.box, &lr, &isect);
    unsigned char hit = 0;
    if (isect.penned <= 0.0f || isect.contained <= 0.0f)
    {
        hit = 1;
    }
    return hit;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xCollideFastInit(_xScene* sc)
{
    iCollideFastInit(sc);
}

// TODO (94%): in the OBB case the original moves the loads of r->origin and
// r->max_t/min_t ahead of the stores to lr (see docs/compiler.md).
unsigned int xRayHitsBoundFast(const _xRay3* r, const _xBound* b)
{
    if (b->type == 1)
    {
        return xRayHitsSphereFast(r, &b->sph);
    }
    if (b->type == 4)
    {
        return xRayHitsOBBFast(r, b);
    }
    if (b->type == 2)
    {
        return xRayHitsBoxFast(r, &b->box.box);
    }
    if (b->type == 3)
    {
        return xRayHitsCylinderFast(r, &b->cyl);
    }
    return 0;
}

unsigned int xRayHitsEnvFast(_xRay3* r, _xEnv* env)
{
    return iRayHitsEnvFast(r, env);
}

unsigned int xRayHitsSceneFast(_xRay3* r, _xScene* sc)
{
    xQCData q;
    signed int i;

    xQuickCullForRay(&q, r);

    for (i = 0; i < sc->num_stats; i++)
    {
        _xEnt* ent = sc->stats[i];
        if (ent->chkby && xQuickCullIsects(&q, &ent->bound.qcd) && xRayHitsBoundFast(r, &ent->bound))
        {
            if (ent->collLev == 5)
            {
                if (iRayHitsModelFast(r, ent->model))
                {
                    return 1;
                }
            }
            else
            {
                return 1;
            }
        }
    }

    for (i = 0; i < sc->num_dyns; i++)
    {
        _xEnt* ent = sc->dyns[i];
        if (ent->chkby && xQuickCullIsects(&q, &ent->bound.qcd) && xRayHitsBoundFast(r, &ent->bound))
        {
            if (ent->collLev == 5)
            {
                if (iRayHitsModelFast(r, ent->model))
                {
                    return 1;
                }
            }
            else
            {
                return 1;
            }
        }
    }

    for (i = 0; i < sc->num_npcs; i++)
    {
        _xEnt* ent = sc->npcs[i];
        if (ent->chkby && xQuickCullIsects(&q, &ent->bound.qcd) && xRayHitsBoundFast(r, &ent->bound))
        {
            return 1;
        }
    }

    return iRayHitsEnvSpecialFast(r, sc->env);
}
